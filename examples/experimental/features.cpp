// TODO: autodiff on inputs, for callables and shaders
// TODO: partial evaluation of callables through substitution methods
// TODO: external constant specialization
// TODO: atomics
// TODO: fix optimization...

#include <littlevk/littlevk.hpp>

#include <common/io.hpp>

#include <ire.hpp>
#include <rexec.hpp>

using namespace jvl;
using namespace jvl::ire;
using namespace jvl::rexec;

MODULE(features);

// TODO: type check for soft/concrete types... layout_from only for concrete types
// TODO: generic is anything,
// 	but solid_t aggregates must take only concrete_generics
//      and soft_t aggregates can take anything, defaults to solid_t is all concrete

// TODO: l-value propagation
// TODO: get line numbers for each invocation if possible?
// using source location if available

// Type safe options: shaders as functors...

// In a separate module, author a shader module
// which defines an environment of available
// resources and the corresponding methods under that scope...

// Example:
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() {
		return layout_from("MVP",
			verbatim_field(model),
			verbatim_field(view),
			verbatim_field(proj));
	}
};

template <size_t Offset>
$rexec_callable(Projection, $push_constant(MVP, Offset))
{
	using self = Projection;

	// Forward declaring to define an interface...
	// $rexec_subroutine_ext(vec4, apply, vec3 p) {
	$rexec_subroutine(vec4, apply, vec3 p) {
		$return $constants.project(p);
	};
};

$rexec_vertex(VShader,
	$layout_in(vec3, 0),
	$layout_in(vec3, 1),
	$layout_out(vec3, 0),
	$push_constant(MVP, 0)
)
{
	$rexec_subroutine(vec3, value) {
		$return vec3(12, 13, 14);
	};

	$rexec_entrypoint(main) {
		// ...such that the REXEC can still be used appropriately
		gl_Position = $use(Projection <0>).apply($lin(0));
	};
};

$rexec_fragment(FShader,
	$layout_in(vec3, 0),
	$layout_out(vec4, 0)
)
{
	$rexec_entrypoint(main) {
		$lout(0) = vec4(0.5 + 0.5 * $lin(0), 1);
	};
};

// TODO: for partial procedures, need to return
// rexec_procedure when specializing...

// TODO: remake solid_t <T> and start soft_t <T>

using S = solid_t <MVP>;

struct Tmp {
	vec4 b;
	f32 y;

	auto layout() {
		return layout_from("Tmp",
			verbatim_field(b),
			verbatim_field(y));
	}
};

using U = solid_t <Tmp>;
using V = solid_t <vec3>;

// using Bytes = std::vector <uint8_t>;

// template <padded_type ... Ts>
// struct soft_padded : Bytes {
// 	// TODO: split into solid and last type...
// };

int main()
{
	{
		auto glsl = link(VShader::main).generate_glsl();
		io::display_lines("GLSL", glsl);
		// VShader::main.display_assembly();
	}

	{
		auto glsl = link(FShader::main).generate_glsl();
		io::display_lines("GLSL", glsl);
		// FShader::main.display_assembly();
	}

	// Find a suitable physical device
	auto predicate = [&](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, {});
	};

	auto phdev = littlevk::pick_physical_device(predicate);

	auto device = littlevk::device(phdev, 0, 1, { VK_KHR_SWAPCHAIN_EXTENSION_NAME });
	auto deallocator = littlevk::Deallocator(device);

	vk::RenderPass renderpass = littlevk::RenderPassAssembler(device, deallocator)
		.add_attachment(littlevk::default_color_attachment(vk::Format::eR8G8B8A8Snorm))
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.done();

	auto group = TraditionalPipelineGroup(VShader::main, FShader::main);
	group.compile(device, renderpass);
	// TODO: group.make_vertex_buffer(...)
	using G = decltype(group);
}
