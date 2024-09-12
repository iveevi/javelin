#include "logging.hpp"
#include "thunder/atom.hpp"
#include "thunder/buffer.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/properties.hpp"
#include "thunder/qualified_type.hpp"

namespace jvl::thunder {
	
MODULE(classify-atoms);

// Overload lookup methods; forward declarations
static QualifiedType lookup_intrinsic_overload(const IntrinsicOperation &, const std::vector <QualifiedType> &);
static QualifiedType lookup_operation_overload(const OperationCode &, const std::vector <QualifiedType> &);

// TODO: const ref return
QualifiedType Buffer::classify(index_t i) const
{
	// Caching
	if (types[i])
		return types[i];

	Atom atom = atoms[i];

	switch (atom.index()) {

	case Atom::type_index <TypeInformation> ():
	{
		auto &type_field = atom.as <TypeInformation> ();

		std::optional <PlainDataType> pd;
		if (type_field.item != bad)
			pd = PlainDataType(type_field.item);
		if (type_field.down != -1)
			pd = PlainDataType(type_field.down);

		if (pd) {
			if (type_field.next != -1)
				return StructFieldType(pd.value(), type_field.next);
			else
				return pd.value();
		}

		return QualifiedType::nil();
	}

	case Atom::type_index <Primitive> ():
		return QualifiedType::primitive(atom.as <Primitive> ().type);

	case Atom::type_index <Qualifier> ():
	{
		auto &qualifier = atom.as <Qualifier> ();
		
		QualifiedType decl = classify(qualifier.underlying);
		if (qualifier.kind == arrays) {
			if (decl.is <StructFieldType> ())
				decl = QualifiedType::concrete(qualifier.underlying);

			return QualifiedType::array(decl, qualifier.numerical);
		}
		
		if (auto pd = decl.get <PlainDataType> ()) {
			if (pd->is <PrimitiveType> ())
				return decl;
		}

		return QualifiedType::concrete(qualifier.underlying);
	}

	case Atom::type_index <Construct> ():
	{
		index_t t = atom.as <Construct> ().type;

		QualifiedType qt = classify(t);
		if (qt.is <PlainDataType> ())
			return qt;

		return QualifiedType::concrete(t);
	}

	case Atom::type_index <Call> ():
		return classify(atom.as <Call> ().type);
	
	case Atom::type_index <Operation> ():
	{
		auto &operation = atom.as <Operation> ();
		auto args = expand_list_types(operation.args);
		auto result = lookup_operation_overload(operation.code, args);
		if (!result)
			JVL_ABORT("failed to find overload for operation: {}", atom);
		return result;
	}

	case Atom::type_index <Intrinsic> ():
	{
		auto &intrinsic = atom.as <Intrinsic> ();
		auto args = expand_list_types(intrinsic.args);
		auto result = lookup_intrinsic_overload(intrinsic.opn, args);
		if (!result)
			JVL_ABORT("failed to find overload for intrinsic: {}", atom);
		return result;
	}

	case Atom::type_index <Swizzle> ():
	{
		auto &swz = atom.as <Swizzle> ();
		QualifiedType decl = classify(swz.src);
		PlainDataType plain = decl.remove_qualifiers();
		JVL_ASSERT(plain.is <PrimitiveType> (),
			"swizzle should only operate on vector types, "
			"but operand is {} with type {}",
			atoms[swz.src], decl.to_string());

		PrimitiveType swizzled = swizzle_type_of(plain.as <PrimitiveType> (), swz.code);

		return QualifiedType::primitive(swizzled);
	}

	case Atom::type_index <Load> ():
	{
		auto &load = atom.as <Load> ();
		QualifiedType qt = classify(load.src);
		if (load.idx == -1)
			return qt;

		switch (qt.index()) {

		case QualifiedType::type_index <PlainDataType> ():
		{
			auto &pd = qt.as <PlainDataType> ();
			JVL_ASSERT(pd.is <index_t> (), "cannot load field/element from primitive type: {}", qt);

			index_t concrete = pd.as <index_t> ();
			index_t left = load.idx;
			while (true) {
				JVL_ASSERT(concrete != -1, "load attempting to access out of bounds field");
				auto &atom = atoms[concrete];
				JVL_ASSERT(atom.is <TypeInformation> (), "expected type information, instead got: {}", atom);
				auto &ti = atom.as <TypeInformation> ();
				if ((--left) > 0)
					concrete = ti.next;
				else
					break;
			}

			return types[concrete].remove_qualifiers();
		}

		}

		dump();
		JVL_ABORT("unfinished implementation for load: {}", atom);
	}
	
	case Atom::type_index <ArrayAccess> ():
	{
		auto &access = atom.as <ArrayAccess> ();
		QualifiedType qt = classify(access.src);
		
		auto pd = qt.get <PlainDataType> ();
		if (pd && pd->is <index_t> ())
			qt = classify(pd->as <index_t> ());

		JVL_ASSERT(qt.is <ArrayType> (),
			"array accesses must operate on array "
			"types, but source is of type {}", qt);

		return qt.as <ArrayType> ().base();
	}
	
	case Atom::type_index <Returns> ():
		return classify(atom.as <Returns> ().type);

	case Atom::type_index <List> ():
	case Atom::type_index <Store> ():
	case Atom::type_index <Branch> ():
		return QualifiedType::nil();

	default:
		break;
	}

	JVL_ABORT("failed to classify instruction: {}", atom);
}

// TODO: binary operation specialization
// Overload for primitive type operations/intrinsics
struct overload {
	QualifiedType result;
	std::vector <QualifiedType> args;

	template <typename ... Args>
	static overload from(PrimitiveType result, const Args &...args) {
		std::vector <QualifiedType> list { QualifiedType::primitive(args)... };
		return overload(QualifiedType::primitive(result), list);
	}

	static std::string type_decls_to_string(const std::vector <QualifiedType> &args) {
		std::string result = "(";
		for (size_t i = 0; i < args.size(); i++) {
			auto &td = args[i];

			result += td.to_string();
			if (i + 1 < args.size())
				result += ", ";
		}

		return result + ")";
	}
};
	
using overload_list = std::vector <overload>;

overload_list concat(const overload_list &A, const overload_list &B)
{
	overload_list C = A;
	for (auto &ovl : B)
		C.push_back(ovl);
	return C;
}

template <typename T>
struct overload_table : public wrapped::hash_table <T, overload_list> {
	using wrapped::hash_table <T, overload_list> ::hash_table;

	QualifiedType lookup(const T &key, const std::vector <QualifiedType> &args) const {
		const char *const *name_table = nullptr;
		if constexpr (std::same_as <T, IntrinsicOperation>)
			name_table = tbl_intrinsic_operation;
		else if constexpr (std::same_as <T, OperationCode>)
			name_table = tbl_operation_code;

		JVL_ASSERT(this->contains(key),
			"overload table does not contain entry for ${}",
			name_table[key]);
		
		// TODO: JVL_DEBUG_INFO that is enabled only when the module's name is listed
		// JVL_INFO("finding overload {} for ${}",
		// 	overload::type_decls_to_string(args),
		// 	name_table[key]);

		auto &overloads = this->at(key);
		for (auto &ovl : overloads) {
			// JVL_INFO("\tcomparing {} with overload {}",
			// 	overload::type_decls_to_string(args),
			// 	overload::type_decls_to_string(ovl.args));

			if (ovl.args.size() != args.size())
				continue;

			bool success = true;
			for (size_t i = 0; i < args.size(); i++) {
				if (ovl.args[i] != args[i]) {
					success = false;
					break;
				}
			}

			if (success)
				return ovl.result;
		}

		// TODO: warning as error option
		JVL_WARNING("no matching overload {} found for ${}",
			overload::type_decls_to_string(args),
			name_table[key]);

		// TODO: note diagnostics
		JVL_INFO("available overloads for ${}:", name_table[key]);
		for (auto &ovl : overloads) {
			fmt::println("  {} -> {}",
				overload::type_decls_to_string(ovl.args),
				ovl.result.to_string());
		}

		return QualifiedType();
	}
};

// Overload table for intrinsics
static QualifiedType lookup_intrinsic_overload(const IntrinsicOperation &key, const std::vector <QualifiedType> &args)
{
        static const overload_table <IntrinsicOperation> table {
		// Trigonometric functions
                { sin, { overload::from(f32, f32) } },
                { cos, { overload::from(f32, f32) } },
                { tan, { overload::from(f32, f32) } },

		// Inverse trigonometric functions
                { asin, { overload::from(f32, f32) } },
                { acos, { overload::from(f32, f32) } },
                { atan, { overload::from(f32, f32) } },

		// Powering functions
                { sqrt, { overload::from(f32, f32) } },
                { exp, { overload::from(f32, f32) } },
                { pow, { overload::from(f32, f32, f32) } },

		// Limiting functions
                { clamp, {
                        overload::from(f32, f32, f32, f32)
                } },
                
		{ min, {
                        overload::from(f32, f32, f32)
                } },
                
		{ max, {
                        overload::from(f32, f32, f32)
                } },
		
		{ fract, {
                        overload::from(f32, f32),
                        overload::from(vec2, vec2),
                        overload::from(vec3, vec3),
                        overload::from(vec4, vec4),
                } },

		// Vector operations
                { dot, {
                        overload::from(f32, vec2, vec2),
                        overload::from(f32, vec3, vec3),
                        overload::from(f32, vec4, vec4),
                } },
                
		{ cross, {
                        overload::from(vec3, vec3, vec3),
                } },
		
		{ normalize, {
                        overload::from(vec3, vec3),
                } },
		
		{ reflect, {
                        overload::from(vec3, vec3, vec3),
                } },
                
		// GLSL casting intrinsics
		{ glsl_floatBitsToUint, {
                        overload::from(u32, f32),
                        overload::from(uvec2, vec2),
                        overload::from(uvec3, vec3),
                        overload::from(uvec4, vec4),
                } },
                
		{ glsl_uintBitsToFloat, {
                        overload::from(f32, u32),
                        overload::from(vec2, uvec2),
                        overload::from(vec3, uvec3),
                        overload::from(vec4, uvec4),
                } },
        };

        return table.lookup(key, args);
}

// Overload table for operations
static QualifiedType lookup_operation_overload(const OperationCode &key, const std::vector <QualifiedType> &args)
{
	static const overload_list arithmetic_overloads {
		overload::from(i32, i32, i32),
		overload::from(u32, u32, u32),
		overload::from(f32, f32, f32),
		
		overload::from(vec2, vec2, vec2),
		overload::from(vec3, vec3, vec3),
		overload::from(vec4, vec4, vec4),

		overload::from(ivec3, ivec3, i32),
		overload::from(ivec3, i32, i32),

		overload::from(uvec3, uvec3, u32),
		overload::from(uvec3, u32, u32),
		
		overload::from(vec3, vec3, f32),
		overload::from(vec3, f32, vec3),
	};

	static const overload_list matrix_multiplication_overloads {
		overload::from(vec2, mat2, vec2),
		overload::from(vec3, mat3, vec3),
		overload::from(vec4, mat4, vec4),
	};
	
	static const overload_list bitwise_operator_overloads {
		overload::from(i32, i32, i32),
		overload::from(ivec2, ivec2, ivec2),
		overload::from(ivec3, ivec3, ivec3),
		overload::from(ivec4, ivec4, ivec4),
		overload::from(u32, u32, u32),
		overload::from(uvec2, uvec2, uvec2),
		overload::from(uvec2, uvec2, u32),
		overload::from(uvec3, uvec3, uvec3),
		overload::from(uvec3, uvec3, u32),
		overload::from(uvec4, uvec4, uvec4),
		overload::from(uvec4, uvec4, u32),
	};

	static const overload_list comparison_overloads {
		overload::from(boolean, i32, i32),
		overload::from(boolean, f32, f32),
		overload::from(boolean, u32, u32),
	};

        static const overload_table <OperationCode> table {
		{ unary_negation, {
			overload::from(f32, f32),
			overload::from(vec2, vec2),
			overload::from(vec3, vec3),
			overload::from(vec4, vec4),
		} },

		{ addition, arithmetic_overloads },
		{ subtraction, arithmetic_overloads },
		{ multiplication, concat(arithmetic_overloads, matrix_multiplication_overloads) },
		{ division, arithmetic_overloads },

		{ modulus, {
			// TODO: legalize floating point modulus?
			overload::from(i32, i32, i32),
			overload::from(u32, u32, u32),
		} },

		{ bool_or, { overload::from(boolean, boolean, boolean) } },
		{ bool_and, { overload::from(boolean, boolean, boolean) } },
		
		{ bit_and, bitwise_operator_overloads },
		{ bit_or, bitwise_operator_overloads },
		{ bit_xor, bitwise_operator_overloads },
		
		{ bit_shift_left, {
			overload::from(i32, i32, i32),
			overload::from(u32, u32, u32),
			overload::from(uvec3, uvec3, u32),
		} },
		
		{ bit_shift_right, {
			overload::from(i32, i32, i32),
			overload::from(u32, u32, u32),
			overload::from(uvec3, uvec3, u32),
		} },

		{ cmp_ge, comparison_overloads },
		{ cmp_geq, comparison_overloads },
		{ cmp_le, comparison_overloads },
		{ cmp_leq, comparison_overloads },
		{ equals, comparison_overloads },
		{ not_equals, comparison_overloads },
	};

	return table.lookup(key, args);
}

} // namespace jvl::thunder