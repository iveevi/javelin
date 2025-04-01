#include "common/application.hpp"
#include "common/imgui.hpp"

#include "shaders.hpp"

$entrypoint(blit)
{
	layout_out <vec2> uv(0);

	array <vec2> vertices = std::array <vec2, 6> {
		vec2(0, 0),
		vec2(0, 1),
		vec2(1, 0),
		vec2(0, 1),
		vec2(1, 0),
		vec2(1, 1)
	};

	vec2 v = vertices[gl_VertexIndex % 6];
	uv = v;
	v = 2 * v - 1;
	v.y = -v.y;

	gl_Position = vec4(v, 0, 1);
};

struct Uniforms {
	f32 time;
	vec3 resolution;

	auto layout() {
		return phantom_layout_from("Uniforms",
			verbatim_field(time),
			verbatim_field(resolution));
	}
};

struct Shadertoy {
	f32 iTime;
	vec3 iResolution;

	void load() {
		push_constant <Uniforms> constants;

		iTime = constants.time;
		iResolution = constants.resolution;
	}
};

struct Creation : Shadertoy {
	void main(vec4 &fragColor, const vec2 &fragCoord) {
		#define r iResolution.xy()
		#define t iTime

		vec3 c;
		// f32 l, z = t;
		f32 l = clone(t);
		f32 z = clone(t);

		$for (i, range(0, 3)) {
			vec2 uv;
			vec2 p = fragCoord.xy() / r;
			uv = p;
			p -= vec2(0.5);
			p.x *= r.x / r.y;
			z += 0.07;
			l = length(p);
			uv += p / l * (sin(z) + 1.0) * abs(sin(l * 9.0 - z - z));

			f32 val = 0.01 / length(mod(uv, 1.0f) - 0.5);
			
			$if (i == 0) {
				c.x = val;
			} $elif (i == 1) {
				c.y = val;
			} $else {
				c.z = val;
			};
		};

		fragColor = vec4(c / l, 1);

		#undef r
		#undef t
	}
};

struct Warping : Shadertoy {
	const mat2 m = mat2(0.80,  0.60, -0.60,  0.80);

	// TODO: make each of these a callable
	f32 noise(in <vec2> p)
	{
		return sin(p.x) * sin(p.y);
	}

	f32 fbm4(vec2 p)
	{
		f32 f = 0.0;
		f += 0.5000 * noise(p); p = m * p * 2.02;
		f += 0.2500 * noise(p); p = m * p * 2.03;
		f += 0.1250 * noise(p); p = m * p * 2.01;
		f += 0.0625 * noise(p);
		return f / 0.9375;
	}

	f32 fbm6(vec2 p)
	{
		f32 f = 0.0;
		f += 0.500000*(0.5+0.5 * noise(p)); p = m*p*2.02;
		f += 0.250000*(0.5+0.5 * noise(p)); p = m*p*2.03;
		f += 0.125000*(0.5+0.5 * noise(p)); p = m*p*2.01;
		f += 0.062500*(0.5+0.5 * noise(p)); p = m*p*2.04;
		f += 0.031250*(0.5+0.5 * noise(p)); p = m*p*2.01;
		f += 0.015625*(0.5+0.5 * noise(p));
		return f / 0.96875;
	}

	vec2 fbm4_2(vec2 p)
	{
		return vec2(fbm4(p), fbm4(p+vec2(7.8)));
	}

	vec2 fbm6_2(vec2 p)
	{
		return vec2(fbm6(p + vec2(16.8)), fbm6(p + vec2(11.5)));
	}

	f32 ftn(vec2 q, out <vec4> ron)
	{
		q += 0.03*sin(vec2(0.27, 0.23) * iTime + length(q) * vec2(4.1, 4.3));
		vec2 o = fbm4_2(0.9 * q);
		o += 0.04*sin(vec2(0.12, 0.14) * iTime + length(o));
		vec2 n = fbm6_2(3.0 * o);
		ron = vec4(o, n);
		f32 f = 0.5 + 0.5 * fbm4(1.8 * q + 6.0 * n);
		return mix(f, f * f * f * 3.5, f * abs(n.x));
	}

	void main(vec4 &fragColor, const vec2 &fragCoord) {
		vec2 p = (2.0 * fragCoord - iResolution.xy()) / iResolution.y;
		f32 e = 2.0f / iResolution.y;

		vec4 on = vec4(0.0);
		f32 f = ftn(p, on);

		vec3 col = vec3(0.0);
		col = mix(vec3(0.2, 0.1, 0.4), vec3(0.3, 0.05, 0.05), f );
		col = mix(col, vec3(0.9, 0.9, 0.9), dot(on.zw(),on.zw()) );
		col = mix(col, vec3(0.4, 0.3, 0.3), 0.2 + 0.5f *on.y *on.y);
		col = mix(col, vec3(0.0, 0.2, 0.4), 0.5 * smoothstep(1.2f, 1.3f, abs(on.z) + abs(on.w)));
		col = clamp(col * f * 2.0, 0.0f, 1.0f);

		vec4 kk;
	 	vec3 nor = normalize(vec3(ftn(
			p + vec2(e, 0.0), kk) - f,
			2.0 * e,
			ftn(p + vec2(0.0, e), kk) - f));

		vec3 lig = normalize(vec3(0.9, 0.2, -0.4));
		f32 dif = clamp(0.3 + 0.7 * dot(nor, lig), 0.0f, 1.0f);
		vec3 lin = vec3(0.70,0.90,0.95) * (nor.y * 0.5f + 0.5f) + vec3(0.15,0.10,0.05) * dif;

		col *= 1.2 * lin;
		col = 1.0 - col;
		col = 1.1 * col * col;
		
		fragColor = vec4(col, 1.0);
	}
};

template <typename T>
auto display()
{
	$entrypoint(main) {
		layout_in  <vec2>  uv       (0);
		layout_out <vec4>  fragment (0);

		T shader;
		shader.load();
		shader.main(fragment, uv);
	};

	return main;
}

struct Application : BaseApplication {
	littlevk::Pipeline active;
	vk::RenderPass render_pass;
	std::vector <vk::Framebuffer> framebuffers;

	Application() : BaseApplication("Shadertoy", { VK_KHR_SWAPCHAIN_EXTENSION_NAME }) {
		// Configure render pass
		render_pass = littlevk::RenderPassAssembler(resources.device, resources.dal)
			.add_attachment(littlevk::default_color_attachment(resources.swapchain.format))
			.add_subpass(vk::PipelineBindPoint::eGraphics)
				.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
				.done();
		
		// Configure framebuffers
		littlevk::FramebufferGenerator generator {
			resources.device, render_pass,
			resources.window.extent,
			resources.dal
		};

		for (size_t i = 0; i < resources.swapchain.images.size(); i++)
			generator.add(resources.swapchain.image_views[i]);

		framebuffers = generator.unpack();

		// Configure ImGui
		configure_imgui(resources, render_pass);
	}

	~Application() {
		shutdown_imgui();
	}

	// Configuration and loading from arguments
	void configure(argparse::ArgumentParser &parser) override {

	}

	void preload(const argparse::ArgumentParser &parser) override {
		// TODO: some way to obtain URL and then do it?
		JVL_INFO("compiling shadertoy example \"{}\"", "plain");

		auto vs = blit;
		auto fs = display <Creation> ();

		auto vs_spv = link(vs).generate(Target::spirv_binary_via_glsl, Stage::vertex);
		auto fs_spv = link(fs).generate(Target::spirv_binary_via_glsl, Stage::fragment);

		auto fs_glsl = link(fs).generate_glsl();

		io::display_lines("FRAGMENT", fs_glsl);

		auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
			.code(vs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eVertex)
			.code(fs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eFragment);

		active = littlevk::PipelineAssembler <littlevk::PipelineType::eGraphics>
			(resources.device, resources.window, resources.dal)
			.with_render_pass(render_pass, 0)
			.with_shader_bundle(bundle)
			.with_push_constant <solid_t <Uniforms>> (vk::ShaderStageFlagBits::eFragment)
			.cull_mode(vk::CullModeFlagBits::eNone);
	}

	// Render loop methods
	void render(const vk::CommandBuffer &cmd, uint32_t index, uint32_t) override {
		auto &extent = resources.window.extent;

		// Configure the rendering extent
		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(resources.window.extent));

		littlevk::RenderPassBeginInfo(1)
			.with_render_pass(render_pass)
			.with_framebuffer(framebuffers[index])
			.with_extent(extent)
			.clear_color(0, std::array <float, 4> { 0, 0, 0, 1 })
			.begin(cmd);

		solid_t <Uniforms> uniforms;
		uniforms.get <0> () = glfwGetTime();
		uniforms.get <1> () = glm::vec3(extent.width, extent.height, 1);

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, active.handle);
		cmd.pushConstants <solid_t <Uniforms>> (active.layout, vk::ShaderStageFlagBits::eFragment, 0, uniforms);
		cmd.draw(6, 1, 0, 0);

		imgui(cmd);

		cmd.endRenderPass();
	}

	void imgui(const vk::CommandBuffer &cmd) {
		ImGuiRenderContext context(cmd);

		ImGui::Begin("Shadertoy Rendering Options");
		{
		}
		ImGui::End();
	}

	void resize() override {

	}
};

APPLICATION_MAIN()