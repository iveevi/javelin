#include "shaders.hpp"

// Shader kernels for the sphere rendering
void vertex()
{
	layout_in <vec3> position(0);

	layout_out <vec3> out_position(0);
	layout_out <f32> out_speed(1);
	layout_out <vec2> out_range(2);

	read_only_buffer <unsized_array <vec3>> translations(0);
	read_only_buffer <unsized_array <vec3>> velocities(1);
	
	push_constant <MVP> view_info;

	vec3 translate = translations[gl_InstanceIndex];
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