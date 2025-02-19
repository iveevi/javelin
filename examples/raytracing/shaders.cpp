#include "shaders.hpp"

////////////////////////////
// Target display shaders //
////////////////////////////

Procedure <void> quad = procedure <void> ("main") << []()
{
	array <vec4> locations = std::array <vec4, 6> {
		vec4(-1, -1, 0, 0),
		vec4(1, -1, 1, 0),
		vec4(-1, 1, 0, 1),
		vec4(-1, 1, 0, 1),
		vec4(1, -1, 1, 0),
		vec4(1, 1, 1, 1),
	};

	layout_out <vec2> uv(0);

	vec4 v = locations[gl_VertexIndex % 6];
	gl_Position = vec4(v.xy(), 1 - 0.0001f, 1);	
	uv = v.zw();
};

Procedure <void> blit = procedure <void> ("main") << []()
{
	layout_in <vec2> uv(0);

	layout_out <vec4> fragment(0);

	sampler2D sampler(0);

	fragment = vec4(ire::texture(sampler, uv).xyz(), 1);
};

/////////////////////////
// Ray tracing shaders //
/////////////////////////

Procedure <void> ray_generation = procedure <void> ("main") << []()
{
	ray_payload <vec3> payload(0);

	accelerationStructureEXT tlas(0);

	write_only <image2D> image(1);

	push_constant <ViewFrame> rayframe;
	
	vec2 center = vec2(gl_LaunchIDEXT.xy()) + vec2(0.5);
	vec2 uv = center / vec2(imageSize(image));
	vec3 ray = rayframe.at(uv);

	traceRayEXT(tlas,
		gl_RayFlagsOpaqueEXT,
		0xFF,
		0, 0, 0,
		rayframe.origin, 1e-3,
		ray, 1e10,
		0);

	vec4 color = vec4(payload, 1.0);

	imageStore(image, ivec2(gl_LaunchIDEXT.xy()), color);
};

Procedure <void> ray_closest_hit = procedure <void> ("main") << []()
{
	// TODO: scalar buffers...
	// TODO: buffer references...
	ray_payload_in <vec3> payload(0);

	hit_attribute <vec2> barycentrics;

	// TODO: debug printf and strings...

	payload = vec3(1.0f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
};

Procedure <void> ray_miss = procedure <void> ("main") << []()
{
	ray_payload_in <vec3> payload(0);

	payload = vec3(1);
};
