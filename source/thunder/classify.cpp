#include "thunder/atom.hpp"
#include "thunder/buffer.hpp"
#include "logging.hpp"
#include "thunder/enumerations.hpp"

namespace jvl::thunder {
	
MODULE(buffer-classify);

// TODO: binary operation specialization
struct overload {
	TypeDecl result;
	std::vector <TypeDecl> args;

	template <typename ... Args>
	static overload from(TypeDecl result, const Args &...args) {
		std::vector <TypeDecl> list { args... };
		return overload(result, list);
	}

	static std::string type_decls_to_string(const std::vector <TypeDecl> &args) {
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

template <typename T>
struct overload_table : public wrapped::hash_table <T, overload_list> {
	using wrapped::hash_table <T, overload_list> ::hash_table;

	TypeDecl lookup(const T &key, const std::vector <TypeDecl> &args) const {
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

		return TypeDecl();
	}
};

// Overload table for intrinsics
// TODO: put at the end of the file
static auto overload_intrinsics()
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

        return table;
}

// TODO: const ref
TypeDecl Buffer::classify(index_t i) const
{
	// Caching
	if (types.contains(i))
		return types.at(i);

	Atom atom = pool[i];

	switch (atom.index()) {

	case Atom::type_index <TypeField> ():
	{
		auto &type_field = atom.as <TypeField> ();
		if (type_field.item != bad)
			return TypeDecl(type_field.item, type_field.next);
		if (type_field.down != -1)
			return TypeDecl(type_field.down, type_field.next);

		return TypeDecl();
	}

	case Atom::type_index <Primitive> ():
		return TypeDecl(atom.as <Primitive> ().type);

	case Atom::type_index <Global> ():
	{
		index_t type = atom.as <Global> ().type;
		TypeDecl decl = classify(type);
		if (decl.is_struct_field())
			return TypeDecl(type);

		return decl;
	}

	case Atom::type_index <Construct> ():
		// TODO: atom.type()
		return classify(atom.as <Construct> ().type);

	case Atom::type_index <Call> ():
		return classify(atom.as <Call> ().type);

	case Atom::type_index <Operation> ():
		// TODO: use overload table
		return classify(atom.as <Operation> ().type);

	case Atom::type_index <Intrinsic> ():
	{
		auto &intrinsic = atom.as <Intrinsic> ();
		auto args = expand_list_types(intrinsic.args);
		auto result = overload_intrinsics().lookup(intrinsic.opn, args);
		if (!result) {
			dump();
			JVL_ABORT("failed to get correct overload for {}", atom);
		}
		return result;
	}

	case Atom::type_index <Swizzle> ():
	{
		auto &swz = atom.as <Swizzle> ();
		TypeDecl decl = classify(swz.src);
		JVL_ASSERT(vector_type(decl.primitive),
			"swizzle should only operate on vector types, "
			"but operand is {} with type {}",
			pool[swz.src], decl.to_string());

		return TypeDecl(swizzle_type_of(decl.primitive, swz.code));
	}

	case Atom::type_index <Load> ():
	{
		auto &load = atom.as <Load> ();
		TypeDecl decl = classify(load.src);
		if (load.idx == -1)
			return decl;

		JVL_ASSERT(!decl.is_primitive(),
			"cannot load from primitive variables");

		index_t i = decl.concrete;
		index_t left = load.idx;
		do {
			decl = types.at(i);
			i = decl.next;
		} while ((--left) > 0);

		if (decl.is_primitive())
			return TypeDecl(decl.primitive);

		return TypeDecl(decl.concrete);
	}

	default:
		return TypeDecl();

	}

	return TypeDecl();
}

} // namespace jvl::thunder