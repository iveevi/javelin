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

struct Hit {
	vec3 color;
	vec3 position;
	vec3 normal;
	boolean missed;

	auto layout() {
		return layout_from("Hit",
			verbatim_field(color),
			verbatim_field(position),
			verbatim_field(normal),
			verbatim_field(missed));
	}
};

Procedure <void> ray_generation = procedure <void> ("main") << []()
{
	ray_payload <Hit> hit(0);
	ray_payload <boolean> shadow(1);

	accelerationStructureEXT tlas(0);

	writeonly <image2D> image(1);

	push_constant <Constants> constants;
	
	vec2 center = vec2(gl_LaunchIDEXT.xy()) + vec2(0.5);
	vec2 uv = center / vec2(imageSize(image));
	vec3 ray = constants.at(uv);

	traceRayEXT(tlas,
		gl_RayFlagsOpaqueEXT,
		0xFF,
		0, 0, 0,
		constants.origin, 1e-3,
		ray, 1e10,
		0);

	vec4 color = vec4(hit.color, 1.0);

	// Shadow testing
	$if(!hit.missed);
	{
		shadow = true;
		
		vec3 offset = hit.position + 1e-3 * hit.normal;

		traceRayEXT(tlas,
			gl_RayFlagsOpaqueEXT
			| gl_RayFlagsTerminateOnFirstHitEXT
			| gl_RayFlagsSkipClosestHitShaderEXT,
			0xFF,
			0, 0, 1,
			offset, 1e-3,
			constants.light, 1e10,
			1);

		hit.color = 0.5 + 0.5 * hit.normal;
		hit.color *= 0.5 + 0.5 * (1.0f - f32(shadow));
	}
	$end();

	// TODO: more semantic traceRaysEXT...
	// traceRayEXT(tlas, flags, mask, nullptr, ..., shadow_miss, ray..., shadow)
	
	imageStore(image, ivec2(gl_LaunchIDEXT.xy()), color);
};

struct Vertex {
	vec3 position;
	vec3 normal;

	auto layout() {
		return layout_from("Vertex",
			verbatim_field(position),
			verbatim_field(normal));
	}
};

Procedure <void> primary_closest_hit = procedure <void> ("main") << []()
{
	using Triangles = scalar <buffer_reference <unsized_array <ivec3>>>;
	using Vertices = scalar <buffer_reference <unsized_array <Vertex>>>;

	ray_payload_in <Hit> hit(0);

	buffer <unsized_array <Reference>> references(2);

	hit_attribute <vec2> bary;

	u32 iid = gl_InstanceCustomIndexEXT;
	u32 pid = gl_PrimitiveID;

	Reference ref = references[iid];

	Triangles triangles = Triangles(ref.triangles);
	Vertices vertices = Vertices(ref.vertices);

	ivec3 tri = triangles[pid];

	Vertex v0 = vertices[tri.x];
	Vertex v1 = vertices[tri.y];
	Vertex v2 = vertices[tri.z];

	vec3 b = vec3(1.0f - bary.x - bary.y, bary.x, bary.y);
	vec3 p = b.x * v0.position + b.y * v1.position + b.z * v2.position;
	vec3 n = b.x * v0.normal + b.y * v1.normal + b.z * v2.normal;

	// position = vec3(gl_ObjectToWorldEXT * vec4(p, 1));
	hit.position = (gl_ObjectToWorldEXT * vec4(p, 1)).xyz();
	hit.normal = normalize(n);
	hit.color = b;
	hit.missed = false;
};

Procedure <void> primary_miss = procedure <void> ("main") << []()
{
	ray_payload_in <Hit> hit(0);

	hit.position = vec3(0);
	hit.color = vec3(1);
	hit.missed = true;
};

Procedure <void> shadow_miss = procedure <void> ("main") << []()
{
	ray_payload_in <boolean> shadow(1);

	shadow = false;
};
