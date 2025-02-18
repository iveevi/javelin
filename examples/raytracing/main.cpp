#include "common/aperature.hpp"
#include "common/camera_controller.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/extensions.hpp"
#include "common/imgui.hpp"
#include "common/imported_asset.hpp"
#include "common/transform.hpp"
#include "common/vulkan_resources.hpp"
#include "common/application.hpp"
#include "common/util.hpp"

#include <ire.hpp>

using namespace jvl;
using namespace jvl::ire;

struct ViewFrame {
	vec3 origin;
	vec3 lower_left;
	vec3 horizontal;
	vec3 vertical;

	vec3 at(vec2 uv) {
		return normalize(lower_left + uv.x * horizontal + uv.y * vertical - origin);
	}

	auto layout() {
		return layout_from("Frame",
			verbatim_field(origin),
			verbatim_field(lower_left),
			verbatim_field(horizontal),
			verbatim_field(vertical));
	}
};

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
		gl_RayFlagsOpaqueEXT | gl_RayFlagsCullNoOpaqueEXT,
		0xFF,
		0, 0, 0,
		rayframe.origin, 1e-3,
		ray, 1e10,
		0);

	vec4 color = vec4(pow(payload, vec3(1/2.2)), 1.0);

	imageStore(image, ivec2(gl_LaunchIDEXT.xy()), color);
};

Procedure <void> ray_closest_hit = procedure <void> ("main") << []()
{
	ray_payload_in <vec3> payload(0);

	hit_attribute <vec2> barycentrics;

	payload = vec3(1.0f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
};

Procedure <void> ray_miss = procedure <void> ("main") << []()
{
	ray_payload_in <vec3> payload(0);

	payload = vec3(0);
};

struct Application : CameraApplication {
	littlevk::Pipeline pipeline;

	vk::RenderPass render_pass;
	DefaultFramebufferSet framebuffers;

	Application() : CameraApplication("Raytracing", {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		}, features_include, features_activate)
	{
		render_pass = littlevk::RenderPassAssembler(resources.device, resources.dal)
			.add_attachment(littlevk::default_color_attachment(resources.swapchain.format))
			.add_attachment(littlevk::default_depth_attachment())
			.add_subpass(vk::PipelineBindPoint::eGraphics)
				.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
				.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
				.done();

		configure_imgui(resources, render_pass);
		
		// Framebuffer manager
		framebuffers.resize(resources, render_pass);
	
		compile_pipeline();
	}

	static void features_include(VulkanFeatureChain &features) {
		features.add <vk::PhysicalDeviceRayTracingPipelineFeaturesKHR> ();
	}

	static void features_activate(VulkanFeatureChain &features) {
		for (auto &ptr : features) {
			switch (ptr->sType) {
			feature_case(vk::PhysicalDeviceRayTracingPipelineFeaturesKHR)
				.setRayTracingPipeline(true);
				break;
			default:
				break;
			}
		}
	}

	void compile_pipeline() {
		std::string rgen_shader = link(ray_generation).generate_glsl();
		std::string rchit_shader = link(ray_closest_hit).generate_glsl();
		std::string rmiss_shader = link(ray_miss).generate_glsl();

		dump_lines("RAY GENERATION", rgen_shader);
		dump_lines("RAY CLOSEST HIT", rchit_shader);
		dump_lines("RAY MISS", rmiss_shader);
		
		auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
			.source(rgen_shader, vk::ShaderStageFlagBits::eRaygenKHR)
			.source(rchit_shader, vk::ShaderStageFlagBits::eClosestHitKHR)
			.source(rmiss_shader, vk::ShaderStageFlagBits::eMissKHR);
	}

	void render(const vk::CommandBuffer &cmd, uint32_t index) override {
		// Configure the rendering extent
		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(resources.window.extent));

		littlevk::RenderPassBeginInfo(2)
			.with_render_pass(render_pass)
			.with_framebuffer(framebuffers[index])
			.with_extent(resources.window.extent)
			.clear_color(0, std::array <float, 4> { 1, 1, 1, 1 })
			.clear_depth(1, 1)
			.begin(cmd);

		imgui(cmd);
		
		cmd.endRenderPass();
	}
	
	void imgui(const vk::CommandBuffer &cmd) {
		ImGuiRenderContext context(cmd);

		ImGui::Begin("Ray Tracing: Options");
		{
		}
		ImGui::End();
	}

	void resize() override {
		framebuffers.resize(resources, render_pass);
	}
};

APPLICATION_MAIN()