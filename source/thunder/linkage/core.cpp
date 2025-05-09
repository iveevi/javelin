#include <fstream>

#include <vulkan/vulkan.hpp>

#include "common/logging.hpp"

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

		globals.samplers[binding] = expandable_local_layout_type(
			fidx,
			bidx,
			qualifier.kind,
			{},
			std::nullopt
		);

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
	} break;

	case shared:
	{
		size_t id = qualifier.numerical;
		local_layout_type bf(fidx, qualifier.underlying, qualifier.kind);
		globals.shared[id] = bf;
	} break;

	case writeonly:
	case readonly:
	case scalar:

	case format_rgba32f:
	case format_rgba16f:
	{
		// TODO: method...

		// Find base qualifier (i.e. image or buffer)
		auto lower = function.atoms[bidx];

		while (lower.is <Qualifier> ()) {
			auto &qt = lower.as <Qualifier> ();

			// Acceptable chained qualifiers
			if (format_kind(qt.kind)
					|| qt.kind == writeonly
					|| qt.kind == readonly
					|| qt.kind == scalar)
				lower = function.atoms[qt.underlying];
			else
				break;
		}

		// Compatibility checks for qualifiers and targets
		static const std::set <QualifierKind> image_compatible {
			writeonly,
			readonly,
		};

		auto &kind = qualifier.kind;
		if (auto decl = lower.get <Qualifier> ()) {
			if (image_kind(decl->kind) && (image_compatible.contains(kind) || format_kind(kind))) {
				globals.images[decl->numerical].extra.insert(kind);
				break;
			} else if (decl->kind == storage_buffer) {
				globals.buffers[decl->numerical].extra.insert(kind);
				break;
			} else if (kind == scalar && decl->kind == buffer_reference) {
				globals.references[decl->numerical].extra.insert(kind);
				break;
			}
		}

		JVL_ABORT("unexpected atom for {} qualifier:\n{}", tbl_qualifier_kind[kind], lower);
	} break;

	case task_payload:
		globals.special[task_payload][-1] = special_type(fidx, bidx);
		break;

	case hit_attribute:
		globals.special[hit_attribute][-1] = special_type(fidx, bidx);
		break;

	case acceleration_structure:
	case ray_tracing_payload:
	case ray_tracing_payload_in:
		globals.special[qualifier.kind][qualifier.numerical] = special_type(fidx, bidx);
		break;

	case arrays:
	{
		auto &lower = qualifier.underlying;
		auto &atom = function.atoms[lower];

		if (atom.is <Qualifier> ()) {
			auto &uqt = atom.as <Qualifier> ();

			if (sampler_kind(uqt.kind)) {
				JVL_INFO("array of samplers: {}", qualifier.to_assembly_string());

				// Override the original sampler
				globals.samplers[uqt.numerical].size = qualifier.numerical;
			}
		}
	} break;

	default:
		break;
	}
}

void LinkageUnit::process_function_intrinsic(Function &function, size_t index, Index i, const Intrinsic &intr)
{
	// TODO: method?
	if (intr.opn == thunder::layout_local_size) {
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
	} else if (intr.opn == thunder::layout_mesh_shader_sizes) {
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

// TODO: check for duplicate names... shouldnt exist in linkage unit
LinkageUnit::function_result_t LinkageUnit::process_function(const Function &ftn)
{
	std::set <Index> referenced;

	// TODO: run validation here as well
	Index fidx = functions.size();

	functions.emplace_back(ftn);
	types.emplace_back();

	auto &function = functions.back();
	auto &map = types.back();

	for (Index bidx : function.marked) {
		// Sanity check
		JVL_ASSERT(bidx >= 0, "bad index @{} in synthesized list for function", bidx);

		auto &atom = function[bidx];

		// Get the return type of the function
		if (atom.is <Return> ())
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

		// Extensions for special primitives
		if (atom.is <TypeInformation> ()) {
			auto &ti = atom.as <TypeInformation> ();
			if (ti.item == u64)
				extensions.insert("GL_EXT_shader_explicit_arithmetic_types_int64");
		}

		// Checking for structs used by the function
		auto qt = function.types[bidx];
		if (qt.is <StructFieldType> ())
			process_function_aggregate(map, function, fidx, bidx, qt);

		// TODO: static initializers (e.g. arrays and constants)
	}

	// Gather extensions from instructions
	// TODO: enums for extensions/capabilities/features
	for (size_t i = 0; i < ftn.pointer; i++) {
		auto &atom = ftn.atoms[i];

		// From intrinsics...
		// TODO: methods?
		if (auto intrinsic = atom.get <Intrinsic> ()) {
			static std::map <thunder::IntrinsicOperation, std::string> registered {
				{ thunder::nonuniformEXT, "GL_EXT_nonuniform_qualifier" },
				{ thunder::cast_to_uint64, "GL_EXT_shader_explicit_arithmetic_types_int64" },
				{ thunder::glsl_subgroupShuffle, "GL_KHR_shader_subgroup_shuffle" },
				{ thunder::set_mesh_outputs, "GL_EXT_mesh_shader" },
				{ thunder::emit_mesh_tasks, "GL_EXT_mesh_shader" },
			};

			auto it = registered.find(intrinsic->opn);
			if (it != registered.end())
				extensions.insert(it->second);
		} else if (auto qualifier = atom.get <Qualifier> ()) {
			static std::map <thunder::QualifierKind, std::string> registered {
				{ thunder::buffer_reference, "GL_EXT_buffer_reference" },
				{ thunder::scalar, "GL_EXT_scalar_block_layout" },
				{ thunder::task_payload, "GL_EXT_mesh_shader" },
				{ thunder::hit_attribute, "GL_EXT_ray_tracing" },
				{ thunder::acceleration_structure, "GL_EXT_raytracing" },
				{ thunder::ray_tracing_payload_in, "GL_EXT_raytracing" },
				{ thunder::ray_tracing_payload, "GL_EXT_raytracing" },
				{ thunder::glsl_LaunchIDEXT, "GL_EXT_raytracing" },
				{ thunder::glsl_LaunchSizeEXT, "GL_EXT_raytracing" },
				{ thunder::glsl_SubgroupInvocationID, "GL_KHR_shader_subgroup_basic" },
			};
			
			auto it = registered.find(qualifier->kind);
			if (it != registered.end())
				extensions.insert(it->second);
		}
	}

	return std::make_pair(fidx, referenced);
}

Index LinkageUnit::add(uint32_t cid, const NamedBuffer &callable)
{
	if (loaded.contains(cid))
		return loaded[cid];

	Function converted {
		callable,
		callable.name,
		cid
	};

	auto [fidx, referenced] = process_function(converted);

	loaded[cid] = fidx;

	std::set <Index> translated;
	for (Index i : referenced) {
		Index j = add(i, TrackedBuffer::cache_load(i));
		translated.insert(j);
	}

	// Mark the dependencies
	dependencies[fidx] = translated;

	return fidx;
}

Index LinkageUnit::add(const TrackedBuffer &callable)
{
	return add(callable.cid, callable);
}

// Translating target stages into Vulkan counterparts
vk::ShaderStageFlagBits to_vulkan(Stage stage)
{
	static constexpr const char *tbl_stage[] {
		"vertex",
		"fragment",
		"compute",
		"mesh",
		"task",
		"ray generation",
		"intersection",
		"any hit",
		"closest hit",
		"miss",
		"callable",
	};

	switch (stage) {
	case Stage::vertex:
		return vk::ShaderStageFlagBits::eVertex;
	case Stage::fragment:
		return vk::ShaderStageFlagBits::eFragment;
	case Stage::compute:
		return vk::ShaderStageFlagBits::eCompute;
	case Stage::mesh:
		return vk::ShaderStageFlagBits::eMeshEXT;
	case Stage::task:
		return vk::ShaderStageFlagBits::eTaskEXT;
	case Stage::ray_generation:
		return vk::ShaderStageFlagBits::eRaygenKHR;
	case Stage::intersection:
		return vk::ShaderStageFlagBits::eIntersectionKHR;
	case Stage::any_hit:
		return vk::ShaderStageFlagBits::eAnyHitKHR;
	case Stage::closest_hit:
		return vk::ShaderStageFlagBits::eClosestHitKHR;
	case Stage::miss:
		return vk::ShaderStageFlagBits::eMissKHR;
	case Stage::callable:
		return vk::ShaderStageFlagBits::eCallableKHR;
	default:
		break;
	}

	JVL_ABORT("unsupported stage {}", tbl_stage[(int) stage]);
}

GeneratedResult LinkageUnit::generate(const Target &target, const Stage &stage) const
{
	static constexpr const char *tbl_targets[] {
		"glsl",
		"slang",
		"cplusplus",
		"cuda",
		"cuda (nvrtc)",
		"spirv (assembly)",
		"spirv (binary)",
		"spirv (binary via glsl)",
	};

	switch (target) {
	case Target::glsl:
		return generate_glsl();
	case Target::cplusplus:
		return generate_cpp();
	case Target::spirv_binary_via_glsl:
		return generate_spirv_via_glsl(to_vulkan(stage));
	default:
		break;
	}

	JVL_ABORT("generation target {} is currently unsupported", tbl_targets[(int) target]);
}

// Serialization
void LinkageUnit::write(const std::filesystem::path &path) const
{
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open()) {
		JVL_ERROR("failed to open file '{}' for writing", path.string());
		return;
	}

	auto write_string = [&](const std::string &str) {
		size_t size = str.size();
		file.write(reinterpret_cast <const char *> (&size), sizeof(size_t));
		file.write(str.data(), size);
	};

	// TODO: other properties

	size_t count = functions.size();
	file.write(reinterpret_cast <const char *> (&count), sizeof(size_t));

	for (auto &ftn : functions) {
		write_string(ftn.name);
		ftn.write(file);
	}

	file.close();
}

void LinkageUnit::write_assembly(const std::filesystem::path &path) const
{
	std::ofstream file(path);
	if (!file.is_open()) {
		JVL_ERROR("failed to open file '{}' for writing", path.string());
		return;
	}

	for (auto &ftn : functions) {
		file << ftn.name << '.' << ftn.cid << ":\n";
		file << ftn.to_string_assembly() << '\n';
	}

	return file.close();
}

} // namespace jvl::thunder
