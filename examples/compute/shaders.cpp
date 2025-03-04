#include "common/cmaps.hpp"
#include "common/util.hpp"
#include "shaders.hpp"

// Shader kernels for the sphere rendering
void vertex()
{
	layout_in <vec3> position(0);

	layout_out <vec3> out_position(0);
	layout_out <f32> out_speed(1);
	layout_out <vec2> out_range(2);

	readonly <buffer <unsized_array <vec3>>> positions(0);
	readonly <buffer <unsized_array <vec3>>> velocities(1);
	
	push_constant <MVP> view_info;

	vec3 translate = positions[gl_InstanceIndex];
	vec3 velocity = velocities[gl_InstanceIndex];
	
	gl_Position = view_info.project_vertex(translate, position);

	out_position = position;
	out_speed = length(velocity);
	out_range = vec2(view_info.smin, view_info.smax);
}

void fragment(vec3 (*cmap)(f32))
{
	layout_in <vec3> position(0);
	layout_in <f32> speed(1);
	layout_in <vec2> range(2);

	layout_out <vec4> fragment(0);

	f32 t = (speed - range.x) / (range.y - range.x);

	fragment = vec4(cmap(t), 1.0f);
}

void integrator()
{
	local_size group(32);

	push_constant <SimulationInfo> info;
	
	buffer <unsized_array <vec3>> positions(0);
	buffer <unsized_array <vec3>> velocities(1);

	u32 tid = gl_GlobalInvocationID.x;

	$if(tid >= info.count);
		$return();
	$end();

	vec3 p = positions[tid];
	vec3 v = velocities[tid];

	vec3 d;
	f32 l;

	d = (p - info.O1);	
	l = max(length(d), 1.0f);
	v = v - info.dt * info.M * d / pow(l, 3.0f);
	
	d = (p - info.O2);	
	l = max(length(d), 1.0f);
	v = v - info.dt * info.M * d / pow(l, 3.0f);

	p += info.dt * v;

	velocities[tid] = v;
	positions[tid] = p;
}

// Debugging
void shader_debug()
{
	static const std::string local = std::filesystem::path(__FILE__).parent_path();

	auto vs_callable = procedure("main") << vertex;
	auto fs_callable = procedure("main") << std::make_tuple(jet) << fragment;
	auto cs_callable = procedure("main") << integrator;

	std::string vertex_shader = link(vs_callable).generate_glsl();
	std::string fragment_shader = link(fs_callable).generate_glsl();
	std::string compute_shader = link(cs_callable).generate_glsl();

	dump_lines("VERTEX", vertex_shader);
	dump_lines("FRAGMENT", fragment_shader);
	dump_lines("COMPUTE", compute_shader);

	vs_callable.graphviz(local + "/vertex.dot");
	fs_callable.graphviz(local + "/fragment.dot");
	cs_callable.graphviz(local + "/compute.dot");

	thunder::optimize(vs_callable);
	thunder::optimize(fs_callable);
	thunder::optimize(cs_callable);

	vs_callable.graphviz(local + "/vertex-optimized.dot");
	fs_callable.graphviz(local + "/fragment-optimized.dot");
	cs_callable.graphviz(local + "/compute-optimized.dot");
}