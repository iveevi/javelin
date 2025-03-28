#include <common/io.hpp>

#include "shaders.hpp"

struct Glyph {
	vec4 bound;
	u64 address;

	auto layout() {
		return layout_from("Glyph",
			verbatim_field(bound),
			verbatim_field(address));
	}
};

Procedure <void> vertex = procedure("main") << []()
{
	layout_out <vec2>       uv          (0);
	layout_out <u64, flat>  address     (1);
	layout_out <vec2>       size        (2);

	push_constant <ivec2>   resolution;

	buffer <unsized_array <Glyph>> bbox (0);

	array <vec2> vertices = std::array <vec2, 6> {
		vec2(0, 0),
		vec2(0, 1),
		vec2(1, 0),
		vec2(0, 1),
		vec2(1, 0),
		vec2(1, 1)
	};

	vec4 box = bbox[gl_InstanceIndex].bound;
	vec2 min = box.xy();
	vec2 max = box.zw();
	vec2 extent = max - min;
	vec2 v = vertices[gl_VertexIndex % 6];
	uv = v;
	v = 2 * (v * extent + min) - 1;
	v.y = -v.y;
	gl_Position = vec4(v, 0, 1);
	address = bbox[gl_InstanceIndex].address;
	size = extent * vec2(resolution);
};

Procedure winding_contribution_solve = procedure <i32> ("winding_contribution_solve")
	<< [](vec2 p0, vec2 p1, vec2 p2, vec2 position)
{
	f32 x = position.x;
	f32 x0 = p0.x;
	f32 x1 = p1.x;
	f32 x2 = p2.x;

	// Quick rejection: if the ray is completely to the right of the curve
	$if (x > max(x0, max(x1, x2)));
		$return(0);
	$end();

	f32 y = position.y;
	f32 y0 = p0.y;
	f32 y1 = p1.y;
	f32 y2 = p2.y;

	// Quick rejection: if the ray is completely above or below the curve
	$if (y < min(y0, min(y1, y2)) || y > max(y0, max(y1, y2)));
		$return(0);
	$end();

	// Solve the quadratic equation a*t^2 + b*t + c = 0 for intersections
	f32 a = y0 - 2.0 * y1 + y2; // Quadratic coefficient
	f32 b = 2.0 * (y1 - y0);   // Linear coefficient
	f32 c = y0 - y;            // Constant term

	// Handle degenerate case: if the curve is effectively linear in the y-direction
	$if (abs(a) < 1e-6);
	{
		$if (abs(b) < 1e-6);
			$return(0); // The curve is horizontal; no contribution
		$end();

		f32 t = -c / b;
		$if (t < 0.0 || t > 1.0);
			$return(0); // Intersection is outside the curve
		$end();

		f32 xt = (1.0 - t) * (1.0 - t) * x0 + 2.0 * (1.0 - t) * t * x1 + t * t * x2;

		// return (xt >= x) ? 1 : 0; // Check if the intersection is to the right of the test point
		// TODO: select operation...
		$if (xt >= x);
			$return(1);
		$else();
			$return(0);
		$end();
	}
	$end();

	// Compute the discriminant of the quadratic equation
	f32 discriminant = b * b - 4.0 * a * c;
	$if (discriminant < 0.0);
		$return(0); // No real roots, no intersection
	$end();

	f32 sqrt_d = sqrt(discriminant);

	// Solve for the two roots
	f32 t0 = (-b - sqrt_d) / (2.0 * a);
	f32 t1 = (-b + sqrt_d) / (2.0 * a);

	// Check the range of the roots and their contributions
	int contribution = 0;

	$if (t0 >= 0.0 && t0 <= 1.0);
	{
		f32 tt0 = 1.0 - t0;
		f32 xt0 = tt0 * tt0 * x0 + 2.0 * tt0 * t0 * x1 + t0 * t0 * x2;

		$if (xt0 >= x);
			contribution += 1;
		$end();
	}
	$end();

	$if (t1 >= 0.0 && t1 <= 1.0);
	{
		f32 tt1 = 1.0 - t1;
		f32 xt1 = tt1 * tt1 * x0 + 2.0 * tt1 * t1 * x1 + t1 * t1 * x2;

		$if (xt1 >= x);
			contribution += 1;
		$end();
	}
	$end();

	$return(contribution);
};

struct Curves {
	i32 size;
	unsized_array <vec2> beziers;

	auto layout() {
		return phantom_layout_from("Curves",
			verbatim_field(size),
			verbatim_field(beziers));
	}
};

Procedure inside = procedure <i32> ("inside") << [](vec2 uv) -> i32
{
	layout_in <u64, flat> address(1);

	auto curves = buffer_reference <Curves> (address);

	i32 winding_number = 0;

	auto it = range <i32> (0, curves.size, 1);

	auto i = $for(it);
	{
		vec2 p0 = curves.beziers[3 * i + 0];
		vec2 p1 = curves.beziers[3 * i + 1];
		vec2 p2 = curves.beziers[3 * i + 2];

		winding_number += winding_contribution_solve(p0, p1, p2, uv);
	}
	$end();

	return i32(winding_number % 2 == 1);
};

Procedure <void> fragment = procedure("main") << []()
{
	layout_in  <vec2>       uv          (0);
	layout_in  <vec2>       resolution  (2);
	layout_out <vec4>       fragment    (0);

	i32 count = 0;
	i32 samples = 5;

	$if (samples == 1);
	{
		count += inside(uv);
	}
	$elif (samples == 2);
	{
		// Four rook sampling
		count += inside(uv + vec2(-0.25, 0.75) / resolution);
		count += inside(uv + vec2(0.75, 0.25) / resolution);
		count += inside(uv + vec2(0.25, -0.75) / resolution);
		count += inside(uv + vec2(-0.75, -0.25) / resolution);
	}
	$else();
	{
		auto it = range <i32> (0, samples, 1);

		auto i = $for(it);
		{
			auto j = $for(it);
			{
				f32 u = f32(i) / f32(samples - 1) - 0.5;
				f32 v = f32(j) / f32(samples - 1) - 0.5;
				count += inside(uv + vec2(u, v) / resolution);
			}
			$end();
		}
		$end();
	}
	$end();

	fragment = vec4(f32(count) / f32(samples * samples));
};

// Debugging
void shader_debug()
{
	std::string vertex_shader = link(vertex).generate_glsl();
	std::string winding_shader = link(winding_contribution_solve).generate_glsl();
	std::string inside_shader = link(inside).generate_glsl();
	std::string fragment_shader = link(fragment).generate_glsl();

	io::display_lines("VERTEx", vertex_shader);
	io::display_lines("WINDING", winding_shader);
	io::display_lines("INSIDE", inside_shader);
	io::display_lines("FRAGMENT", fragment_shader);
}
