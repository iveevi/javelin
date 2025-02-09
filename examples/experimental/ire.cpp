// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <thunder/opt.hpp>
#include <thunder/linkage_unit.hpp>
#include <constants.hpp>

using namespace jvl;
using namespace jvl::ire;

struct payload {
	u32 pindex;
	u32 resolution;

	auto layout() const {
		return uniform_layout("payload",
			named_field(pindex),
			named_field(resolution));
	}
};

struct config {
	mat4 model;
	mat4 view;
	mat4 proj;
	u32 resolution;

	vec4 project(vec3 v) const {
		return proj * (view * (model * vec4(v, 1)));
	}

	auto layout() const {
		return uniform_layout("config",
			named_field(model),
			named_field(view),
			named_field(proj),
			named_field(resolution));
	}
};

auto task = procedure <void> ("main") << []()
{
	push_constant <config> pc;

	task_payload <payload> payload;

	payload.pindex = gl_GlobalInvocationID.x;
	payload.resolution = pc.resolution;

	u32 groups = (payload.resolution - 1 + 6) / 7;

	EmitMeshTasksEXT(groups, groups, 1);
};

auto eval = procedure("eval") << [](const vec2 &uv) -> vec3
{
	const uint32_t FEATURE_SIZE = 20;
	const uint32_t ENCODING_LEVELS = 8;
	const uint32_t FFIN = FEATURE_SIZE + 3 * 2 * ENCODING_LEVELS;
	const uint32_t MSIZE = FFIN;

	array <float> A(MSIZE);

	array <vec4> B(16);

	return vec3(uv, 1.0f);
};

// TODO: problems with arrays of one-element structs...

// TODO: move to shaders/mesh.hpp
struct __list_gl_MeshPerVertexEXT {
	struct gl_MeshPerVertexEXT {
		vec4 gl_Position;
		f32 gl_PointSize;

		// TODO: needs to be special or phantom

		explicit gl_MeshPerVertexEXT() = default;

		auto layout() const {
			// TODO: specification!
			return uniform_layout("gl_MeshPerVertexEXT",
				named_field(gl_Position),
				named_field(gl_PointSize));
		}

		gl_MeshPerVertexEXT(const cache_index_t &c) {
			layout().remove_const().ref_with(c);
		}
	};

	using element = gl_MeshPerVertexEXT;

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <gl_MeshPerVertexEXT> ().id;
		thunder::index_t qualifier = em.emit_qualifier(type, -1, thunder::arrays);
		thunder::index_t intr = em.emit_qualifier(qualifier, -1, thunder::glsl_intrinsic_gl_MeshVerticesEXT);
		return cache_index_t::from(intr);
	}
	
	template <integral_native U>
	element operator[](const U &index) const {
		MODULE(array);

		auto &em = Emitter::active;
		native_t <int32_t> location(index);
		thunder::index_t l = location.synthesize().id;
		thunder::index_t c = em.emit_array_access(synthesize().id, l);
		return cache_index_t::from(c);
	}

	template <integral_arithmetic U>
	element operator[](const U &index) const {
		MODULE(array);

		auto &em = Emitter::active;
		thunder::index_t l = index.synthesize().id;
		thunder::index_t c = em.emit_array_access(synthesize().id, l);
		return element(cache_index_t::from(c));
	}
};

static const __list_gl_MeshPerVertexEXT gl_MeshVerticesEXT;

struct __list_gl_PrimitiveTriangleIndicesEXT {
	using element = uvec3;

	cache_index_t synthesize() const {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <uvec3> ().id;
		thunder::index_t qualifier = em.emit_qualifier(type, -1, thunder::arrays);
		thunder::index_t intr = em.emit_qualifier(qualifier, -1, thunder::glsl_intrinsic_gl_PrimitiveTriangleIndicesEXT);
		return cache_index_t::from(intr);
	}
	
	template <integral_native U>
	element operator[](const U &index) const {
		MODULE(array);

		auto &em = Emitter::active;
		native_t <int32_t> location(index);
		thunder::index_t l = location.synthesize().id;
		thunder::index_t c = em.emit_array_access(synthesize().id, l);
		return cache_index_t::from(c);
	}

	template <integral_arithmetic U>
	element operator[](const U &index) const {
		MODULE(array);

		auto &em = Emitter::active;
		thunder::index_t l = index.synthesize().id;
		thunder::index_t c = em.emit_array_access(synthesize().id, l);
		return element(cache_index_t::from(c));
	}
};

static const __list_gl_PrimitiveTriangleIndicesEXT gl_PrimitiveTriangleIndicesEXT;

auto mesh = procedure <void> ("main") << []()
{
	const uint32_t WORK_GROUP_SIZE = 8;

	task_payload <payload> payload;

	local_size(WORK_GROUP_SIZE, WORK_GROUP_SIZE);

	mesh_shader_size(64, 98);
	
	push_constant <config> pc;

	layout_out <unsized_array <vec3>> vertices(0);
	layout_out <unsized_array <u32>> pindex(1);

	const uint32_t MAX_QSIZE = WORK_GROUP_SIZE - 1;

	uvec2 offset = uvec2(gl_LocalInvocationID.x, gl_LocalInvocationID.y)
		+ MAX_QSIZE * uvec2(gl_WorkGroupID.x, gl_WorkGroupID.y);

	cond(offset.x > payload.resolution || offset.y > payload.resolution);
		returns();
	end();
	
	uvec2 offset_triangles = MAX_QSIZE * uvec2(gl_WorkGroupID.x, gl_WorkGroupID.y);

	u32 total_qsize = payload.resolution - 1;
	u32 qwidth = min(MAX_QSIZE, total_qsize - offset_triangles.x);
	u32 qheight = min(MAX_QSIZE, total_qsize - offset_triangles.y);

	u32 vwidth = qwidth + 1;
	u32 vheight = qheight + 1;

	SetMeshOutputsEXT(vwidth * vheight, 2 * qwidth * qheight);

	vec2 uv = vec2(f32(offset.x), f32(offset.y)) / f32(payload.resolution - 1);

	vec3 v = eval(uv);

	vertices[gl_LocalInvocationIndex] = v;
	pindex[gl_LocalInvocationIndex] = payload.pindex;
	gl_MeshVerticesEXT[gl_LocalInvocationIndex].gl_Position = pc.project(v);

	u32 gli = gl_LocalInvocationIndex;
	cond(gl_LocalInvocationID.x < qwidth && gl_LocalInvocationID.y < qheight);
	{
		u32 prim = 2 * (gl_LocalInvocationID.x + qwidth * gl_LocalInvocationID.y);

		// Assumes that the subgroup size is 32
		u32 gsi = gl_SubgroupInvocationID;

		f32 sign = 1;
		u32 side = gsi + 1;

		cond(side >= 32 || gl_LocalInvocationID.x >= qwidth);
		{
			side = gsi - 1;
			sign *= -1;
		}
		end();

		u32 vert = gsi + WORK_GROUP_SIZE;

		cond(vert >= 32);
		{
			vert = gsi - WORK_GROUP_SIZE;
			sign *= -1;
		}
		end();

		u32 sidevert = gsi + WORK_GROUP_SIZE + 1;

		cond(sidevert >= 32);
			sidevert = gsi - WORK_GROUP_SIZE + 1;
		end();

		cond(sidevert >= 32);
			sidevert = gsi - WORK_GROUP_SIZE - 1;
		end();

		vec3 sv = subgroupShuffle(v, side);
		vec3 vv = subgroupShuffle(v, vert);
		vec3 svv = subgroupShuffle(v, sidevert);

		f32 d0 = length(v - svv);
		f32 d1 = length(sv - vv);

		cond(d0 > d1);
		{
			gl_PrimitiveTriangleIndicesEXT[prim] = uvec3(gli, gli + 1, gli + WORK_GROUP_SIZE);
			gl_PrimitiveTriangleIndicesEXT[prim + 1] = uvec3(gli + 1, gli + WORK_GROUP_SIZE + 1, gli + WORK_GROUP_SIZE);
		}
		elif();
		{
			gl_PrimitiveTriangleIndicesEXT[prim] = uvec3(gli, gli + 1, gli + WORK_GROUP_SIZE + 1);
			gl_PrimitiveTriangleIndicesEXT[prim + 1] = uvec3(gli, gli + WORK_GROUP_SIZE + 1, gli + WORK_GROUP_SIZE);
		}
		end();
	}
	end();
};

int main()
{
	// thunder::opt_transform(task);
	// task.dump();
	// fmt::println("{}", link(task).generate_glsl());
	// link(task).generate_spirv(vk::ShaderStageFlagBits::eTaskEXT);
	
	thunder::opt_transform(mesh);
	mesh.dump();
	fmt::println("{}", link(mesh).generate_glsl());
	link(mesh).generate_spirv(vk::ShaderStageFlagBits::eMeshEXT);
}
