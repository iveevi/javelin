#include "common/color.hpp"
#include "common/util.hpp"

#include "shaders.hpp"

Procedure <void> task = procedure <void> ("main") << []()
{
	task_payload <Payload> payload;

	// TODO: buffer with atomically incremented stats
	readonly <buffer <unsized_array <u32>>> sizes(0);

	u32 idx = gl_GlobalInvocationID.x;

	payload.pid = idx;
	payload.offset = MESHLET_SIZE * idx;
	payload.size = sizes[idx];

	EmitMeshTasksEXT(1, 1, 1);
};

Procedure <void> mesh = procedure <void> ("main") << []()
{
	static constexpr uint32_t GROUP_SIZE = 32;

	local_size(GROUP_SIZE, 1);

	mesh_shader_size(3 * GROUP_SIZE, GROUP_SIZE);
	
	task_payload <Payload> payload;

	push_constant <ViewInfo> view_info;
	
	// TODO: scalar buffer...
	readonly <buffer <unsized_array <vec3>>> positions(1);

	layout_out <unsized_array <vec3>> vertices(0);

	// TODO: per primitive?
	layout_out <unsized_array <u32>, flat> pids(1);

	SetMeshOutputsEXT(3 * GROUP_SIZE, GROUP_SIZE);
	// TODO: fix out of bounds errors with conditional...

	u32 local_base = 3u * gl_LocalInvocationID.x;
	u32 global_base = 3u * (gl_LocalInvocationID.x + payload.offset);

	vertices[local_base + 0] = positions[global_base + 0];
	vertices[local_base + 1] = positions[global_base + 1];
	vertices[local_base + 2] = positions[global_base + 2];
	
	pids[local_base + 0] = payload.pid;
	pids[local_base + 1] = payload.pid;
	pids[local_base + 2] = payload.pid;

	gl_MeshVerticesEXT[local_base + 0].gl_Position = view_info.project(positions[global_base + 0]);
	gl_MeshVerticesEXT[local_base + 1].gl_Position = view_info.project(positions[global_base + 1]);
	gl_MeshVerticesEXT[local_base + 2].gl_Position = view_info.project(positions[global_base + 2]);

	gl_PrimitiveTriangleIndicesEXT[gl_LocalInvocationIndex] = uvec3(local_base, local_base + 1, local_base + 2);
};

array <vec3> hsl_palette(float saturation, float lightness, int N)
{
	std::vector <vec3> palette(N);

	float step = 360.0f / float(N);
	for (int32_t i = 0; i < N; i++) {
		glm::vec3 hsl = glm::vec3(i * step, saturation, lightness);
		glm::vec3 rgb = hsl_to_rgb(hsl);
		palette[i] = vec3(rgb.x, rgb.y, rgb.z);
	}

	return palette;
}

Procedure <void> fragment = procedure <void> ("main") << []()
{
	layout_in <vec3> position(0);
	layout_in <u32, flat> pid(1);
	
	// Resulting fragment color
	layout_out <vec4> fragment(0);

	// Render the normal vectors
	vec3 dU = dFdxFine(position);
	vec3 dV = dFdyFine(position);
	vec3 N = normalize(cross(dV, dU));

	vec3 L = normalize(vec3(1, 1, 1));

	f32 lighting = 0.5 * max(dot(N, L), 0.0f) + 0.5f;

	auto palette = hsl_palette(0.5, 0.8, 32);
	
	vec3 color = lighting * palette[pid % u32(palette.length)];

	fragment = vec4(color, 1);
};

// Debugging
void shader_debug()
{
	static const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path() / ".." / "..";
	static const std::filesystem::path local = root / "output" / "meshlet";
	
	std::filesystem::remove_all(local);
	std::filesystem::create_directories(local);

	std::string task_shader = link(task).generate_glsl();
	std::string mesh_shader = link(mesh).generate_glsl();
	std::string fragment_shader = link(fragment).generate_glsl();

	dump_lines("TASK", task_shader);
	dump_lines("MESH", mesh_shader);
	dump_lines("FRAGMENT", fragment_shader);
	
	task.graphviz(local / "task.dot");
	mesh.graphviz(local / "mesh.dot");
	fragment.graphviz(local / "fragment.dot");

	thunder::optimize(task);
	thunder::optimize(mesh);
	thunder::optimize(fragment);
	
	task.graphviz(local / "task-optimize.dot");
	mesh.graphviz(local / "mesh-optimize.dot");
	fragment.graphviz(local / "fragment-optimize.dot");

	link(task, mesh, fragment).write_assembly(local / "shaders.jvl.asm");
}