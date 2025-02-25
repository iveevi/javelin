#include <bestd/hash_table.hpp>

#include "thunder/atom.hpp"
#include "thunder/c_like_generator.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/qualified_type.hpp"
#include "thunder/tracked_buffer.hpp"

namespace jvl::thunder::detail {

MODULE(c-like-generator);

// TODO: separate file at this point...
static std::optional <std::string> generate_global_reference(const std::vector <Atom> &atoms, const Index &idx)
{
	auto &qualifier = atoms[idx].as <Qualifier> ();

	switch (qualifier.kind) {
	case parameter:
		return fmt::format("_arg{}", qualifier.numerical);

	// GLSL input/output etc. qualifiers
	case layout_in_flat:
	case layout_in_noperspective:
	case layout_in_smooth:
		return fmt::format("_lin{}", qualifier.numerical);
	case layout_out_flat:
	case layout_out_noperspective:
	case layout_out_smooth:
		return fmt::format("_lout{}", qualifier.numerical);

	case push_constant:
		return "_pc";

	case uniform_buffer:
		return fmt::format("_uniform{}", qualifier.numerical);

	case storage_buffer:
		return fmt::format("_buffer{}", qualifier.numerical);

	case shared:
		return fmt::format("_shared{}", qualifier.numerical);

	// Extended qualifiers
	case writeonly:
	case readonly:
	case scalar:
		return generate_global_reference(atoms, qualifier.underlying);

	// GLSL images and samplers
	case iimage_1d:
	case iimage_2d:
	case iimage_3d:
	case uimage_1d:
	case uimage_2d:
	case uimage_3d:
	case image_1d:
	case image_2d:
	case image_3d:
		return fmt::format("_image{}", qualifier.numerical);

	case isampler_1d:
	case isampler_2d:
	case isampler_3d:
	case usampler_1d:
	case usampler_2d:
	case usampler_3d:
	case sampler_1d:
	case sampler_2d:
	case sampler_3d:
		return fmt::format("_sampler{}", qualifier.numerical);

	// Special
	case task_payload:
		return "_task_payload";

	case hit_attribute:
		return "_hit_attribute";
	case ray_tracing_payload:
		return fmt::format("_ray_payload{}", qualifier.numerical);
	case ray_tracing_payload_in:
		return fmt::format("_ray_payload{}", qualifier.numerical);
	case acceleration_structure:
		return fmt::format("_accel{}", qualifier.numerical);

	// GLSL shader stage intrinsics
	case glsl_FragCoord:
		return "gl_FragCoord";
	case glsl_FragDepth:
		return "gl_FragDepth";
	case glsl_InstanceID:
		return "gl_InstanceID";
	case glsl_InstanceIndex:
		return "gl_InstanceIndex";
	case glsl_VertexID:
		return "gl_VertexID";
	case glsl_VertexIndex:
		return "gl_VertexIndex";
	case glsl_GlobalInvocationID:
		return "gl_GlobalInvocationID";
	case glsl_LocalInvocationID:
		return "gl_LocalInvocationID";
	case glsl_LocalInvocationIndex:
		return "gl_LocalInvocationIndex";
	case glsl_WorkGroupID:
		return "gl_WorkGroupID";
	case glsl_WorkGroupSize:
		return "gl_WorkGroupSize";
	case glsl_SubgroupInvocationID:
		return "gl_SubgroupInvocationID";
	case glsl_LaunchIDEXT:
		return "gl_LaunchIDEXT";
	case glsl_LaunchSizeEXT:
		return "gl_LaunchSizeEXT";

	case glsl_Position:
		return "gl_Position";
	case glsl_MeshVerticesEXT:
		return "gl_MeshVerticesEXT";
	case glsl_PrimitiveTriangleIndicesEXT:
		return "gl_PrimitiveTriangleIndicesEXT";
	case glsl_InstanceCustomIndexEXT:
		return "gl_InstanceCustomIndexEXT";
	case glsl_PrimitiveID:
		return "gl_PrimitiveID";
	case glsl_ObjectToWorldEXT:
		return "gl_ObjectToWorldEXT";
	case glsl_WorldRayDirectionEXT:
		return "gl_WorldRayDirectionEXT";

	default:
		break;
	}

	return std::nullopt;
}

static std::string arguments_to_string(const std::vector <std::string> &args)
{
	std::string ret;
	ret += "(";
	for (size_t i = 0; i < args.size(); i++) {
		ret += args[i];
		if (i + 1 < args.size())
			ret += ", ";
	}

	ret += ")";
	return ret;
}

std::string generate_operation(OperationCode code, const std::string &a, const std::string &b)
{
	// Binary operator strings
	static const bestd::hash_table <OperationCode, const char *> operators {
		{ addition,		"+" },
		{ subtraction,		"-" },
		{ multiplication,	"*" },
		{ division,		"/" },

		{ modulus,		"%" },

		{ bit_shift_left,	"<<" },
		{ bit_shift_right,	">>" },
		
		{ bool_and,		"&&" },
		{ bool_or,		"||" },

		{ bit_and,		"&" },
		{ bit_or,		"|" },
		{ bit_xor,		"^" },

		{ cmp_ge,		">" },
		{ cmp_geq,		">=" },
		{ cmp_le,		"<" },
		{ cmp_leq,		"<=" },
		{ equals,		"==" },
		{ not_equals,		"!=" },
	};

	// Handle the special cases
	if (code == unary_negation)
		return fmt::format("-({})", a);
	if (code == bool_not)
		return fmt::format("!({})", a);

	// Should be left with purely binary operations
	JVL_ASSERT(operators.contains(code),
		"no operator symbol found for $({})",
		tbl_operation_code[code]);

	const char *const op = operators.at(code);

	return fmt::format("({} {} {})", a, op, b);
}

c_like_generator_t::c_like_generator_t(const auxiliary_block_t &body)
	: auxiliary_block_t(body), indentation(1) {}

void c_like_generator_t::finish(const std::string &s, bool semicolon)
{
	source += std::string(indentation << 2, ' ') + s + (semicolon ? ";" : "") + "\n";
}

void c_like_generator_t::declare(Index index)
{
	auto t = type_to_string(types[index]);
	int n = local_variables.size();
	std::string var = fmt::format("s{}", n);
	std::string stmt = fmt::format("{} {}{}", t.pre, var, t.post);
	local_variables[index] = var;
	finish(stmt);
}

void c_like_generator_t::define(Index index, const std::string &v)
{
	auto t = type_to_string(types[index]);
	int n = local_variables.size();
	std::string var = fmt::format("s{}", n);
	std::string stmt = fmt::format("{} {}{} = {}", t.pre, var, t.post, v);
	local_variables[index] = var;
	finish(stmt);
}

void c_like_generator_t::assign(int index, const std::string &v)
{
	std::string stmt = fmt::format("{} = {}", reference(index), v);
	finish(stmt);
}

std::string c_like_generator_t::reference(Index index) const
{
	JVL_ASSERT(index != -1, "invalid index passed to ref");

	if (local_variables.count(index))
		return local_variables.at(index);

	const Atom &atom = atoms[index];

	switch (atom.index()) {

	variant_case(Atom, Qualifier):
	{
		auto ref = generate_global_reference(atoms, index);
		if (ref)
			return ref.value();

		JVL_ABORT("failed to generate global reference for qualifier:\n{}", atom);
	} break;

	variant_case(Atom, Construct):
	{
		auto &constructor = atom.as <Construct> ();
		if (constructor.mode == transient)
			return inlined(constructor.type);
	} break;

	variant_case(Atom, Load):
	{
		auto &load = atom.as <Load> ();

		std::string ref = reference(load.src);

		// Default pathway
		if (load.idx == -1)
			return ref;

		std::string accessor = fmt::format(".f{}", load.idx);
		
		// Check for name hints for the field
		if (decorations.used.contains(load.src)) {
			auto id = decorations.used.at(load.src);
			auto th = decorations.all.at(id);
			accessor = "." + th.fields[load.idx];
		} else {
			fmt::println("no decoration for load (@{}) source (@{})", index, load.src);
		}

		return ref + accessor;
	}

	variant_case(Atom, Swizzle):
	{
		auto &swizzle = atom.as <Swizzle> ();
		std::string accessor = tbl_swizzle_code[swizzle.code];
		return reference(swizzle.src) + "." + accessor;
	}

	variant_case(Atom, ArrayAccess):
	{
		auto &access = atom.as <ArrayAccess> ();
		return fmt::format("{}[{}]", inlined(access.src), inlined(access.loc));
	}

	default:
		break;
	}

	// TODO: Could be problematic, its not an actual storage location
	return inlined(index);
}

std::string c_like_generator_t::inlined(Index index) const
{
	JVL_ASSERT(index != -1, "invalid index passed to inlined");

	if (local_variables.count(index))
		return local_variables.at(index);

	const Atom &atom = atoms[index];

	switch (atom.index()) {

	variant_case(Atom, Primitive):
		return atom.as <Primitive> ().value_string();

	variant_case(Atom, Operation):
	{
		auto &operation = atom.as <Operation> ();
		std::string a = inlined(operation.a);
		std::string b = (operation.b == -1) ? "" : inlined(operation.b);
		return generate_operation(operation.code, a, b);
	}

	variant_case(Atom, Intrinsic):
	{
		auto &intrinsic = atom.as <Intrinsic> ();
		auto args = arguments(intrinsic.args);
		return tbl_intrinsic_operation[intrinsic.opn] + arguments_to_string(args);
	}

	variant_case(Atom, Construct):
	{
		auto &constructor = atom.as <Construct> ();
		if (constructor.mode == transient)
			return inlined(constructor.type);

		auto t = type_to_string(types[index]);
		if (constructor.args != -1) {
			auto args = arguments(constructor.args);
			return t.pre + t.post + arguments_to_string(args);
		}

		return t.pre + t.post + "()";
	}

	variant_case(Atom, Call):
	{
		auto &call = atom.as <Call> ();

		auto &buffer = TrackedBuffer::cache_load(call.cid);
		std::string args;
		if (call.args != -1)
			args = arguments_to_string(arguments(call.args));

		return fmt::format("{}{}", buffer.name, args);
	}

	variant_case(Atom, Load):
	variant_case(Atom, Swizzle):
	variant_case(Atom, ArrayAccess):
	variant_case(Atom, Qualifier):
		return reference(index);

	default:
		break;
	}

	JVL_ABORT("failed to inline atom: {} (@{})", atom, index);
}

std::vector <std::string> c_like_generator_t::arguments(Index start) const
{
	std::vector <std::string> args;

	int l = start;
	while (l != -1) {
		Atom h = atoms[l];
		if (!h.is <List> ()) {
			JVL_ABORT("unexpected atom in argument list:\n{}", h.to_string());
		}

		List list = h.as <List> ();
		if (list.item == -1) {
			JVL_ABORT("invalid index (-1) found in list item");
		}

		args.push_back(inlined(list.item));

		l = list.next;
	}

	return args;
}

c_like_generator_t::type_string c_like_generator_t::type_to_string(const QualifiedType &qt) const
{
	switch (qt.index()) {

	variant_case(QualifiedType, NilType):
		return { "void", "" };

	variant_case(QualifiedType, ArrayType):
	{
		auto &at = qt.as <ArrayType> ();
		auto base = type_to_string(at.base());

		std::string array = fmt::format("[{}]", at.size);

		// Unsized arrays
		if (at.size < 0)
			array = "[]";

		return type_string {
			.pre = base.pre,
			.post = fmt::format("{}{}", base.post, array)
		};
	}

	variant_case(QualifiedType, PlainDataType):
	{
		auto &pd = qt.as <PlainDataType> ();
		if (auto p = pd.get <PrimitiveType> ()) {
			return type_string {
				.pre = tbl_primitive_types[*p],
				.post = ""
			};
		}

		Index concrete = pd.as <Index> ();
		if (struct_names.contains(concrete)) {
			return type_string {
				.pre = struct_names.at(concrete),
				.post = ""
			};
		}

		return type_to_string(types[concrete]);
	}

	variant_case(QualifiedType, BufferReferenceType):
	{
		auto &brt = qt.as <BufferReferenceType> ();

		return type_string {
			.pre = fmt::format("BufferReference{}", brt.unique),
			.post = "",
		};
	}

	variant_case(QualifiedType, InArgType):
	{
		auto &in = qt.as <InArgType> ();
		auto base = static_cast <PlainDataType> (in);
		auto info = type_to_string(base);

		return type_string {
			.pre = "in " + info.pre,
			.post = info.post
		};
	}

	variant_case(QualifiedType, OutArgType):
	{
		auto &out = qt.as <OutArgType> ();
		auto base = static_cast <PlainDataType> (out);
		auto info = type_to_string(base);

		return type_string {
			.pre = "out " + info.pre,
			.post = info.post
		};
	}

	variant_case(QualifiedType, InOutArgType):
	{
		auto &inout = qt.as <InOutArgType> ();
		auto base = static_cast <PlainDataType> (inout);
		auto info = type_to_string(base);

		return type_string {
			.pre = "inout " + info.pre,
			.post = info.post
		};
	}

	default:
		break;
	}

	JVL_BUFFER_DUMP_AND_ABORT("failed to resolve type name for {}", qt);
}

// Generators for each kind of instruction
template <>
void c_like_generator_t::generate(const Qualifier &, Index)
{
	// Nothing to do here, linkage should have taken
	// care of generating the necessary structs; their
	// assigned names should all be in struct_names
}

template <>
void c_like_generator_t::generate(const TypeInformation &, Index)
{
	// Same idea here; all globals should be
	// instatiated from the linker side
}

template <>
void c_like_generator_t::generate(const Primitive &primitive, Index index)
{
	define(index, primitive.value_string());
}

template <>
void c_like_generator_t::generate(const Swizzle &swizzle, Index index)
{
	define(index, inlined(index));
}

template <>
void c_like_generator_t::generate(const Operation &operation, Index index)
{
	define(index, inlined(index));
}

template <>
void c_like_generator_t::generate(const Intrinsic &intrinsic, Index index)
{
	// Special cases
	if ((intrinsic.opn == thunder::layout_local_size)
		|| (intrinsic.opn == thunder::layout_mesh_shader_sizes))
		return;

	// Keyword intrinsics
	if (intrinsic.opn == thunder::discard)
		return finish(tbl_intrinsic_operation[intrinsic.opn]);

	auto &qt = types[index];

	bool voided = qt.is <NilType> ();
	if (auto pd = qt.get <PlainDataType> ()) {
		if (auto p = pd->get <thunder::PrimitiveType> ()) {
			voided |= (p == none);
		}
	}
	
	if (voided) {
		// Void return type, so no assignment
		auto args = arguments(intrinsic.args);
		std::string v = tbl_intrinsic_operation[intrinsic.opn] + arguments_to_string(args);
		return finish(v);
	} else {
		auto args = arguments(intrinsic.args);
		auto intr = tbl_intrinsic_operation[intrinsic.opn];
		return define(index, intr + arguments_to_string(args));
	}
}

template <>
void c_like_generator_t::generate(const Construct &construct, Index index)
{
	if (construct.mode == transient)
		return;

	if (construct.args == -1)
		return declare(index);

	define(index, inlined(index));
}

template <>
void c_like_generator_t::generate(const Call &call, Index index)
{
	auto &buffer = TrackedBuffer::cache_load(call.cid);
	std::string args = "()";
	if (call.args != -1)
		args = arguments_to_string(arguments(call.args));

	define(index, buffer.name + args);
}

template <>
void c_like_generator_t::generate(const Store &store, Index)
{
	assign(store.dst, inlined(store.src));
}

template <>
void c_like_generator_t::generate(const Load &load, Index index)
{
	define(index, inlined(index));
}

template <>
void c_like_generator_t::generate(const ArrayAccess &access, Index index)
{
	define(index, inlined(index));
}

template <>
void c_like_generator_t::generate(const Branch &branch, Index)
{
	switch (branch.kind) {

	case control_flow_end:
		indentation--;
		finish("}", false);
		return;

	case conditional_if:
		finish(fmt::format("if ({}) {{", inlined(branch.cond)), false);
		indentation++;
		return;

	case loop_while:
		finish(fmt::format("while ({}) {{", inlined(branch.cond)), false);
		indentation++;
		return;

	case conditional_else_if:
		indentation--;
		finish(fmt::format("}} else if ({}) {{", inlined(branch.cond)), false);
		indentation++;
		return;

	case conditional_else:
		indentation--;
		finish("} else {", false);
		indentation++;
		return;

	case control_flow_skip:
		return finish("continue", true);

	case control_flow_stop:
		return finish("break", true);

	default:
		break;
	}

	JVL_ABORT("failed to generate branch: {}", branch);
}

template <>
void c_like_generator_t::generate(const Returns &returns, Index)
{
	if (returns.value >= 0)
		finish("return " + inlined(returns.value));
	else
		finish("return");
}

// Per-atom generator
void c_like_generator_t::generate(Index i)
{
	auto ftn = [&](auto atom) { return generate(atom, i); };
	return std::visit(ftn, atoms[i]);
}

// General generator
std::string c_like_generator_t::generate()
{
	for (size_t i = 0; i < pointer; i++) {
		if (synthesized.count(i)) {
			// fmt::println("  generating: {}", atoms[i]);
			generate(i);
		}
	}

	return source;
}

} // namespace jvl::thunder::detail
