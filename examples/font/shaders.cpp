#include <common/io.hpp>

#include "app.hpp"

// TODO: use in main as well...
struct Glyph {
	vec4 bound;
	u64 address;

	auto layout() {
		return layout_from("Glyph",
			verbatim_field(bound),
			verbatim_field(address));
	}
};

$entrypoint(vertex)
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

$subroutine(i32, winding_contribution_solve, vec2 p0, vec2 p1, vec2 p2, vec2 position)
{
	f32 x = position.x;
	f32 x0 = p0.x;
	f32 x1 = p1.x;
	f32 x2 = p2.x;

	// Quick rejection: if the ray is completely to the right of the curve
	$if (x > max(x0, max(x1, x2))) {
		$return 0;
	};

	f32 y = position.y;
	f32 y0 = p0.y;
	f32 y1 = p1.y;
	f32 y2 = p2.y;

	// Quick rejection: if the ray is completely above or below the curve
	$if (y < min(y0, min(y1, y2)) || y > max(y0, max(y1, y2))) {
		$return 0;
	};

	// Solve the quadratic equation a*t^2 + b*t + c = 0 for intersections
	f32 a = y0 - 2.0 * y1 + y2; // Quadratic coefficient
	f32 b = 2.0 * (y1 - y0);   // Linear coefficient
	f32 c = y0 - y;            // Constant term

	// Handle degenerate case: if the curve is effectively linear in the y-direction
	$if (abs(a) < 1e-6) {
		$if (abs(b) < 1e-6) {
			$return 0; // The curve is horizontal; no contribution
		};

		f32 t = -c / b;
		$if (t < 0.0 || t > 1.0) {
			$return 0; // Intersection is outside the curve
		};

		f32 xt = (1.0 - t) * (1.0 - t) * x0 + 2.0 * (1.0 - t) * t * x1 + t * t * x2;

		$return $select(xt >= x, 1, 0);
	};

	// Compute the discriminant of the quadratic equation
	f32 discriminant = b * b - 4.0 * a * c;
	$if (discriminant < 0.0) {
		$return 0; // No real roots, no intersection
	};

	f32 sqrt_d = sqrt(discriminant);

	// Solve for the two roots
	f32 t0 = (-b - sqrt_d) / (2.0 * a);
	f32 t1 = (-b + sqrt_d) / (2.0 * a);

	// Check the range of the roots and their contributions
	i32 contribution = 0;

	$if (t0 >= 0.0 && t0 <= 1.0) {
		f32 tt0 = 1.0 - t0;
		f32 xt0 = tt0 * tt0 * x0 + 2.0 * tt0 * t0 * x1 + t0 * t0 * x2;
		contribution += i32(xt0 >= x);
	};

	$if (t1 >= 0.0 && t1 <= 1.0) {
		f32 tt1 = 1.0 - t1;
		f32 xt1 = tt1 * tt1 * x0 + 2.0 * tt1 * t1 * x1 + t1 * t1 * x2;
		contribution += i32(xt1 >= x);
	};

	$return contribution;
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

$subroutine(i32, inside, vec2 uv)
{
	layout_in <u64, flat> address(1);

	auto curves = buffer_reference <Curves> (address);

	i32 winding_number = 0;

	$for (i, range(0, curves.size)) {
		vec2 p0 = curves.beziers[3 * i + 0];
		vec2 p1 = curves.beziers[3 * i + 1];
		vec2 p2 = curves.beziers[3 * i + 2];

		winding_number += winding_contribution_solve(p0, p1, p2, uv);
	};

	$return i32(winding_number % 2 == 1);
};

$partial_entrypoint(fragment, int32_t samples)
{
	layout_in  <vec2>       uv          (0);
	layout_in  <vec2>       resolution  (2);
	layout_out <vec4>       fragment    (0);

	i32 count = 0;

	if (samples == 1) {
		count += inside(uv);
	} else if (samples == 2) {
		// Four rook sampling
		count += inside(uv + vec2(-0.25, 0.75) / resolution);
		count += inside(uv + vec2(0.75, 0.25) / resolution);
		count += inside(uv + vec2(0.25, -0.75) / resolution);
		count += inside(uv + vec2(-0.75, -0.25) / resolution);
	} else {
		// TODO: loop unrolling?
		$for (i, range(0, samples)) {
			$for (j, range(0, samples)) {
				f32 u = f32(i) / f32(samples - 1) - 0.5;
				f32 v = f32(j) / f32(samples - 1) - 0.5;
				count += inside(uv + vec2(u, v) / resolution);
			};
		};
	}

	f32 proportion = f32(count) / f32(samples * samples);

	fragment = vec4(proportion);
};

// Compile pipelines
void Application::compile_pipeline(int32_t samples)
{
	static std::vector <vk::DescriptorSetLayoutBinding> bindings {
		vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex),
	};

	auto fs = fragment(samples);

	auto vs_spv = link(vertex).generate(Target::spirv_binary_via_glsl, Stage::vertex);
	auto fs_spv = link(fs).generate(Target::spirv_binary_via_glsl, Stage::fragment);

	auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
		.code(vs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eVertex)
		.code(fs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eFragment);

	raster = littlevk::PipelineAssembler <littlevk::PipelineType::eGraphics>
		(resources.device, resources.window, resources.dal)
		.with_render_pass(render_pass, 0)
		.with_shader_bundle(bundle)
		.with_dsl_bindings(bindings)
		.with_push_constant <solid_t <ivec2>> (vk::ShaderStageFlagBits::eVertex)
		.cull_mode(vk::CullModeFlagBits::eNone);
}

// Debugging
void Application::shader_debug()
{
	set_trace_destination(root() / ".javelin");

	auto vs = vertex;
	auto fs = fragment(3);

	trace_unit("font", Stage::vertex, vs);
	trace_unit("font", Stage::fragment, fs);
}
