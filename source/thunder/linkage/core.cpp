#include "thunder/enumerations.hpp"
#include "thunder/linkage_unit.hpp"
#include "thunder/properties.hpp"

namespace jvl::thunder {

MODULE(linkage-unit);

// Equality comparison for aggregates
bool Aggregate::operator==(const Aggregate &other) const
{
	// TODO: use a unique id system...
	// TODO: check if either is a default name?
	if (name != other.name)
		return false;
	
	if (phantom != other.phantom)
		return false;

	if (fields.size() != other.fields.size())
		return false;

	for (size_t i = 0; i < fields.size(); i++) {
		if (fields[i] != other.fields[i])
			return false;
	}

	return true;
}

// Functions
Function::Function(const Buffer &buffer, const std::string &name_, size_t cid_)
		: Buffer(buffer), cid(cid_), name(name_) {}

// Linkage unit methods
Index LinkageUnit::new_aggregate(size_t ftn, bool phantom, const std::string &name, const std::vector <Field> &fields)
{
	size_t index = aggregates.size();

	Aggregate aggr {
		.function = ftn,
		.phantom = phantom,
		.name = name,
		.fields = fields
	};

	auto it = std::find(aggregates.begin(), aggregates.end(), aggr);
	if (it == aggregates.end()) {
		aggregates.push_back(aggr);
		return index;
	}

	return std::distance(aggregates.begin(), it);
}

// fidx; function index
// bidx; (atom) buffer index
void LinkageUnit::process_function_qualifier(Function &function, size_t fidx, Index bidx, const Qualifier &qualifier)
{
	if (image_kind(qualifier.kind)) {
		size_t binding = qualifier.numerical;
		globals.images[binding] = local_layout_type(fidx, bidx, qualifier.kind);
		return;
	}

	if (sampler_kind(qualifier.kind)) {
		size_t binding = qualifier.numerical;
		globals.samplers[binding] = local_layout_type(fidx, bidx, qualifier.kind);
		return;
	}

	// Qualifier behaviour; the final type,
	// offset, etc. for a qualified type is
	// that which is specified by the last
	// instruction referring to it
	switch (qualifier.kind) {

	case parameter:
	{
		size_t binding = qualifier.numerical;
		size_t size = std::max(function.args.size(), binding + 1);
		function.args.resize(size);
		function.args[binding] = function.types[bidx];
	} break;

	case push_constant:
	{
		push_constant_info info;
		info.index = bidx;
		info.function = fidx;
		info.kind = thunder::push_constant;
		info.offset = qualifier.numerical;
		info.underlying = qualifier.underlying;

		globals.push_constant = info;
	} break;

	case layout_in_flat:
	case layout_in_noperspective:
	case layout_in_smooth:
	{
		size_t binding = qualifier.numerical;
		local_layout_type lin(fidx, qualifier.underlying, qualifier.kind);
		globals.inputs[binding] = lin;
	} break;

	case layout_out_flat:
	case layout_out_noperspective:
	case layout_out_smooth:
	{
		size_t binding = qualifier.numerical;
		local_layout_type lout(fidx, qualifier.underlying, qualifier.kind);
		globals.outputs[binding] = lout;
	} break;

	case uniform_buffer:
	{
		size_t binding = qualifier.numerical;
		local_layout_type un(fidx, qualifier.underlying, qualifier.kind);
		globals.uniforms[binding] = un;
	} break;

	case storage_buffer:
	{
		size_t binding = qualifier.numerical;

		// May be a redefinition
		// TODO: sanity check to ensure that other properties are the same
		if (globals.buffers.contains(binding))
			JVL_WARNING("redefinition of buffer @{}", binding);
		else
			globals.buffers[binding] = local_layout_type(fidx, bidx, qualifier.kind);
	} break;

	case buffer_reference:
	{
		size_t binding = qualifier.numerical;
		globals.references[binding] = local_layout_type(fidx, bidx, qualifier.kind);
		extensions.insert("GL_EXT_buffer_reference");
	} break;

	case shared:
	{
		size_t id = qualifier.numerical;
		local_layout_type bf(fidx, qualifier.underlying, qualifier.kind);
		globals.shared[id] = bf;
	} break;

	case uint64:
		extensions.insert("GL_EXT_shader_explicit_arithmetic_types_int64");
		break;

	case writeonly:
	case readonly:
	case scalar:
	{
		static const std::set <QualifierKind> image_compatible {
			writeonly,
			readonly,
		};

		auto &kind = qualifier.kind;
		auto &lower = function.atoms[qualifier.underlying];

		if (kind == scalar)
			extensions.insert("GL_EXT_scalar_block_layout");

		if (auto decl = lower.get <Qualifier> ()) {
			if (image_compatible.contains(kind) && image_kind(decl->kind)) {
				globals.images[decl->numerical].extra.insert(kind);
				break;
			} else if (decl->kind == storage_buffer) {
				globals.buffers[decl->numerical].extra.insert(kind);
				break;
			}
		}

		JVL_ABORT("unexpected atom for {} qualifier:\n{}", tbl_qualifier_kind[kind], lower);
	} break;

	case task_payload:
		globals.special[task_payload][-1] = special_type(fidx, bidx);
		extensions.insert("GL_EXT_mesh_shader");
		break;

	case hit_attribute:
		globals.special[hit_attribute][-1] = special_type(fidx, bidx);
		extensions.insert("GL_EXT_ray_tracing");
		break;
	
	case acceleration_structure:
	case ray_tracing_payload:
	case ray_tracing_payload_in:
		globals.special[qualifier.kind][qualifier.numerical] = special_type(fidx, bidx);
		extensions.insert("GL_EXT_ray_tracing");
		break;
	
	case glsl_LaunchIDEXT:
	case glsl_LaunchSizeEXT:
		extensions.insert("GL_EXT_ray_tracing");
		break;

	case glsl_SubgroupInvocationID:
		// TODO: use (portable) enums for extensions
		extensions.insert("GL_KHR_shader_subgroup_basic");
		break;

	// Miscellaneous
	case arrays:
		break;

	case glsl_GlobalInvocationID:
	case glsl_InstanceIndex:
	case glsl_LocalInvocationID:
	case glsl_LocalInvocationIndex:
	case glsl_MeshVerticesEXT:
	case glsl_Position:
	case glsl_PrimitiveTriangleIndicesEXT:
	case glsl_VertexIndex:
	case glsl_WorkGroupID:
	case glsl_WorkGroupSize:
		break;

	default:
		JVL_WARNING("unhandled qualifier in function @{}:\n{}", bidx, qualifier);
		break;
	}
}

void LinkageUnit::process_function_intrinsic(Function &function, size_t index, Index i, const Intrinsic &intr)
{
	switch (intr.opn) {

	case thunder::layout_local_size:
	{
		thunder::Index args = intr.args;

		glm::uvec3 size = glm::uvec3(1, 1, 1);

		int32_t j = 0;
		while (args != -1 && j < 3) {
			Atom local = function[args];

			JVL_ASSERT_PLAIN(local.is <List> ());
			auto list = local.as <List> ();
			local = function[list.item];

			JVL_ASSERT_PLAIN(local.is <Primitive> ());
			auto value = local.as <Primitive> ();

			JVL_ASSERT_PLAIN(value.type == u32);
			size[j++] = value.udata;

			args = list.next;
		}

		local_size = size;
	} break;

	case thunder::layout_mesh_shader_sizes:
	{
		thunder::Index args = intr.args;

		glm::uvec2 size = glm::uvec2(1, 1);

		int32_t j = 0;
		while (args != -1 && j < 2) {
			Atom local = function[args];

			JVL_ASSERT_PLAIN(local.is <List> ());
			auto list = local.as <List> ();
			local = function[list.item];

			JVL_ASSERT_PLAIN(local.is <Primitive> ());
			auto value = local.as <Primitive> ();

			JVL_ASSERT_PLAIN(value.type == u32);
			size[j++] = value.udata;

			args = list.next;
		}

		mesh_shader_size = size;
	} break;

	case thunder::emit_mesh_tasks:
	case thunder::set_mesh_outputs:
		extensions.insert("GL_EXT_mesh_shader");
		break;

	case thunder::glsl_subgroupShuffle:
		extensions.insert("GL_KHR_shader_subgroup_shuffle");
		break;
	
	default:
		break;
	}
}

void LinkageUnit::process_function_aggregate(TypeMap &map, const Function &function, size_t fidx, Index bidx, QualifiedType qt)
{
	std::vector <Field> fields;

	fields.emplace_back(qt.as <StructFieldType> ().base(), "f0");

	do {
		auto &sft = qt.as <StructFieldType> ();
		auto name = fmt::format("f{}", fields.size());
		qt = function.types[sft.next];
		if (qt.is <StructFieldType> ()) {
			auto &sft = qt.as <StructFieldType> ();
			fields.emplace_back(sft.base(), name);
		} else {
			fields.emplace_back(qt, name);
		}
	} while (qt.is <StructFieldType> ());

	JVL_ASSERT(qt.is <NilType> (), "failed to reach nil marker for structure");

	// Remove marker
	fields.pop_back();

	// Check for type hints
	auto &decorations = function.decorations;
	
	std::string name = fmt::format("s{}_t", aggregates.size());
	if (decorations.used.contains(bidx)) {
		auto &id = decorations.used.at(bidx);
		auto &decoration = decorations.all.at(id);
		name = decoration.name;

		for (size_t i = 0; i < fields.size(); i++)
			fields[i].name = decoration.fields[i];
	}

	map[bidx] = new_aggregate(fidx, decorations.phantom.contains(bidx), name, fields);
}

std::set <Index> LinkageUnit::process_function(const Function &ftn)
{
	std::set <Index> referenced;

	// TODO: run validation here as well
	Index fidx = functions.size();

	functions.emplace_back(ftn);
	types.emplace_back();

	auto &function = functions.back();
	auto &map = types.back();

	for (Index bidx : function.synthesized) {
		// Sanity check
		JVL_ASSERT(bidx >= 0, "bad index @{} in synthesized list for function", bidx);

		auto &atom = function[bidx];

		// Get the return type of the function
		if (atom.is <Returns> ())
			function.returns = function.types[bidx];

		// Parameters and global variables
		if (atom.is <Qualifier> ())
			process_function_qualifier(function, fidx, bidx, atom.as <Qualifier> ());

		// Referenced callables
		if (atom.is <Call> ()) {
			auto &call = atom.as <Call> ();
			referenced.insert(call.cid);
		}

		// Checking for special intrinsics
		if (atom.is <Intrinsic> ())
			process_function_intrinsic(function, fidx, bidx, atom.as <Intrinsic> ());

		// Checking for structs used by the function
		auto qt = function.types[bidx];
		if (qt.is <StructFieldType> ())
			process_function_aggregate(map, function, fidx, bidx, qt);

		// TODO: static initializers (e.g. arrays and constants)
	}

	// Mark the dependencies
	dependencies[fidx] = referenced;
	cids[ftn.cid] = fidx;

	return referenced;
}

void LinkageUnit::add(const TrackedBuffer &callable)
{
	if (loaded.contains(callable.cid))
		return;

	Function converted {
		callable,
		callable.name,
		callable.cid
	};

	auto referenced = process_function(converted);

	loaded.insert(callable.cid);
	for (Index i : referenced)
		add(*TrackedBuffer::search_tracked(i));
}

} // namespace jvl::thunder
