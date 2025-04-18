#include <common/io.hpp>

#include "shaders.hpp"

////////////////////////////
// Target display shaders //
////////////////////////////

$entrypoint(quad)
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

$entrypoint(blit)
{
	layout_in <vec2> uv(0);

	layout_out <vec4> fragment(0);

	sampler2D sampler(0);

	fragment = vec4(ire::texture(sampler, uv).xyz(), 1);
};

//////////////////////////
// Path tracing shaders //
//////////////////////////

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

$subroutine(uvec3, pcg3d, uvec3 v)
{
	v = v * 1664525u + 1013904223u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v ^= v >> 16u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	$return v;
};

$subroutine(vec3, random3, inout <vec3> seed)
{
	seed = uintBitsToFloat((pcg3d(floatBitsToUint(seed)) & 0x007FFFFFu) | 0x3F800000u) - 1.0;
	$return seed;
};

$subroutine(vec3, spherical, f32 theta, f32 phi)
{
	$return vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));
};

$subroutine(vec3, randomH2, inout <vec3> seed)
{
	vec3 eta = random3(seed);
	f32 theta = acos(eta.x);
	f32 phi = float(2 * M_PI) * eta.y;
	$return spherical(theta, phi);
};

$subroutine(vec3, rotate, vec3 s, vec3 n)
{
        vec3 w = n;
	vec3 ut = vec3(-w.y, w.x, 0.0);
	vec3 uf = vec3(0.0, -w.z, w.y);
	vec3 u = $select(abs(w.z) < 0.999f, ut, uf);
        vec3 v = cross(w, u);
        $return u * s.x + v * s.y + w * s.z;
};

$entrypoint(ray_generation)
{
	ray_payload <Hit> hit(0);
	ray_payload <boolean> shadow(1);

	accelerationStructureEXT tlas(0);

	format <image2D, rgba32f> image(1);

	push_constant <Constants> constants;

	auto shadowed = [&](vec3 position, vec3 direction) -> boolean {
		shadow = true;

		traceRayEXT(tlas,
			gl_RayFlagsOpaqueEXT
			| gl_RayFlagsTerminateOnFirstHitEXT
			| gl_RayFlagsSkipClosestHitShaderEXT,
			0xFF,
			0, 0, 1,
			position, 1e-3,
			direction, 1e10,
			1);

		return shadow;
	};

	// TODO: fix
	vec3 seed;
	seed = vec3(f32(gl_LaunchIDEXT.x), f32(gl_LaunchIDEXT.y), constants.time);

	vec2 center = vec2(gl_LaunchIDEXT.xy()) + vec2(0.5);
	vec2 uv = center / vec2(imageSize(image));
	vec3 ray = constants.at(uv);
	// TODO: should construct automatically...
	vec3 origin = constants.origin.xyz();

	vec3 beta = vec3(1);
	vec3 color = vec3(0);

	// TODO: compile with fixed depth and samples per pixel per launch
	auto it = _range <u32> (0, 5, 1);

	$for(_, it) {
		traceRayEXT(tlas,
			gl_RayFlagsOpaqueEXT,
			0xFF,
			0, 0, 0,
			origin, 1e-3,
			ray, 1e10,
			0);

		$if (hit.missed) {
			// Missing hits the background light
			color += beta * vec3(2, 2, 1.8);
			_break();
		} $else {
			origin = hit.position + 1e-3 * hit.normal;

			vec3 s = randomH2(seed);
			s = rotate(s, hit.normal);
			ray = s;

			beta *= vec3(1.0) * max(dot(hit.normal, s), 0.0f);
		};
	};

	// TODO: tone mapping
	// TODO: sample count..
	ivec2 idx = ivec2(gl_LaunchIDEXT.xy());
	vec4 old = imageLoad(image, idx);
	vec4 add = (f32(constants.frame) * old + vec4(color, 1.0)) / f32(constants.frame + 1);
	imageStore(image, idx, add);
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

$entrypoint(primary_closest_hit)
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
	p = (gl_ObjectToWorldEXT * vec4(p, 1)).xyz();

	vec3 n = b.x * v0.normal + b.y * v1.normal + b.z * v2.normal;
	n = normalize((n * gl_ObjectToWorldEXT).xyz());

	$if (dot(n, gl_WorldRayDirectionEXT) > 0) {
		n = -n;
	};

	hit.position = p;
	hit.normal = n;
	hit.color = b;
	hit.missed = false;
};

$entrypoint(primary_miss)
{
	ray_payload_in <Hit> hit(0);

	hit.position = vec3(0);
	hit.color = vec3(1);
	hit.missed = true;
};

$entrypoint(shadow_miss)
{
	ray_payload_in <boolean> shadow(1);

	shadow = false;
};

// Debugging
void shader_debug()
{
	static const std::filesystem::path root = std::filesystem::path(__FILE__).parent_path() / ".." / "..";
	static const std::filesystem::path local = root / "output" / "pathtracing";

	std::filesystem::remove_all(local);
	std::filesystem::create_directories(local);

	std::string rgen_shader = link(ray_generation).generate_glsl();
	std::string rchit_shader = link(primary_closest_hit).generate_glsl();
	std::string rmiss_shader = link(primary_miss).generate_glsl();
	std::string smiss_shader = link(shadow_miss).generate_glsl();
	std::string pcg3d_shader = link(pcg3d).generate_glsl();
	std::string random3_shader = link(random3).generate_glsl();
	std::string quad_shader = link(quad).generate_glsl();
	std::string blit_shader = link(blit).generate_glsl();

	io::display_lines("RAY GENERATION", rgen_shader);
	io::display_lines("PRIMARY CLOSEST HIT", rchit_shader);
	io::display_lines("PRIMARY MISS", rmiss_shader);
	io::display_lines("SHADOW MISS", smiss_shader);
	io::display_lines("PCG3D", pcg3d_shader);
	io::display_lines("RANDOM3", random3_shader);
	io::display_lines("QUAD", quad_shader);
	io::display_lines("BLIT", blit_shader);

	ray_generation.graphviz(local / "ray-generation.dot");
	primary_closest_hit.graphviz(local / "primary-closest-hit.dot");
	primary_miss.graphviz(local / "primary-miss.dot");
	shadow_miss.graphviz(local / "shadow-miss.dot");
	pcg3d.graphviz(local / "pcg3d.dot");
	random3.graphviz(local / "random3.dot");
	quad.graphviz(local / "quad.dot");
	blit.graphviz(local / "blit.dot");

	Optimizer::stable.apply(ray_generation);
	Optimizer::stable.apply(primary_closest_hit);
	Optimizer::stable.apply(primary_miss);
	Optimizer::stable.apply(shadow_miss);
	Optimizer::stable.apply(pcg3d);
	Optimizer::stable.apply(random3);
	Optimizer::stable.apply(quad);
	Optimizer::stable.apply(randomH2);
	Optimizer::stable.apply(pcg3d);

	ray_generation.graphviz(local / "ray-generation-optimized.dot");
	primary_closest_hit.graphviz(local / "primary-closest-hit-optimized.dot");
	primary_miss.graphviz(local / "primary-miss-optimized.dot");
	shadow_miss.graphviz(local / "shadow-miss-optimized.dot");
	pcg3d.graphviz(local / "pcg3d-optimized.dot");
	random3.graphviz(local / "random3-optimized.dot");
	quad.graphviz(local / "quad-optimized.dot");
	blit.graphviz(local / "blit-optimized.dot");

	link(ray_generation,
		primary_closest_hit,
		primary_miss,
		shadow_miss,
		pcg3d, random3,
		quad, blit).write_assembly(local / "shaders.jvl.asm");
}
