#include <cassert>
#include <filesystem>

#include <fmt/printf.h>
#include <fmt/std.h>

#include <littlevk/littlevk.hpp>

#include "core/preset.hpp"
#include "core/triangle_mesh.hpp"
#include "gfx/vk_triangle_mesh.hpp"
#include "math_types.hpp"

namespace jvl {

struct Kernel {
	enum {
		eCPU,
		eCUDA,
		eVulkan,
		eOpenGL,
		eMetal,
	} backend;
};

} // namespace jvl

#include "core/aperature.hpp"
#include "core/transform.hpp"

#include <fmt/printf.h>
#include <fmt/format.h>

// Shader sources
struct MVP {
	jvl::float4x4 model;
	jvl::float4x4 view;
	jvl::float4x4 proj;
};

const std::string vertex_shader_source = R"(
#version 450

layout (push_constant) uniform MVP {
	mat4 model;
	mat4 view;
	mat4 proj;
};

layout (location = 0) in vec3 position;

layout (location = 0) out vec3 out_color;

void main()
{
	gl_Position = proj * view * model * vec4(position, 1.0);
	gl_Position.y = -gl_Position.y;
	out_color = position;
}
)";

const std::string fragment_shader_source = R"(
#version 450

layout (location = 0) in vec3 position;

layout (location = 0) out vec4 fragment;

void main()
{
	vec3 dU = dFdx(position);
	vec3 dV = dFdyFine(position);
	vec3 N = normalize(cross(dU, dV));
	fragment = vec4(0.5 + 0.5 * N, 1.0);
}
)";

struct MouseInfo {
	bool left_drag = false;
	bool voided = true;
	float last_x = 0.0f;
	float last_y = 0.0f;
} mouse;

void button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		mouse.left_drag = (action == GLFW_PRESS);
		if (action == GLFW_RELEASE)
			mouse.voided = true;
	}
}

void cursor_callback(GLFWwindow *window, double xpos, double ypos)
{
	auto transform = reinterpret_cast <jvl::core::Transform *> (glfwGetWindowUserPointer(window));

	if (mouse.voided) {
		mouse.last_x = xpos;
		mouse.last_y = ypos;
		mouse.voided = false;
	}

	float xoffset = xpos - mouse.last_x;
	float yoffset = ypos - mouse.last_y;

	mouse.last_x = xpos;
	mouse.last_y = ypos;

	constexpr float sensitivity = 0.0025f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	static float pitch = 0;
	static float yaw = 0;

	if (mouse.left_drag) {
		jvl::float3 horizontal { 0, 1, 0 };
		jvl::float3 vertical = transform->right();

		pitch -= xoffset;
		yaw += yoffset;

		float pi_e = jvl::core::pi <float> /2 - 1e-3f;
		yaw = std::min(pi_e, std::max(-pi_e, yaw));

		transform->rotation = jvl::fquat::euler_angles(yaw, pitch, 0);
	}
}

std::string to_string(const jvl::float4x4 &A)
{
	std::string ret = "matrix[4][4] = {\n";
	for (size_t i = 0; i < 4; i++) {
		ret += "  {";
		for (size_t j = 0; j < 4; j++) {
			ret += fmt::format("{:.4f}", A[i][j]);
			if (j < 3)
				ret += ", ";
		}

		ret += "}";
		if (i < 3)
			ret += ",";
		ret += "\n";
	}

	return ret + "}";
}

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	std::filesystem::path path = argv[1];
	fmt::println("path to scene: {}", path);

	auto preset = jvl::core::Preset::from(path).value();
	// auto tmesh = jvl::core::TriangleMesh::from(preset.geometry[4]).value();

	//////////////////
	// VULKAN SETUP //
	//////////////////

	// Device extensions
	static const std::vector<const char *> EXTENSIONS {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	// Load physical device
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, EXTENSIONS);
	};

	vk::PhysicalDevice phdev = littlevk::pick_physical_device(predicate);

	vk::SurfaceKHR surface;
	littlevk::Window window;

	vk::Extent2D default_extent { 1000, 1000 };

	std::tie(surface, window) = littlevk::surface_handles(default_extent, "javelin");

	littlevk::QueueFamilyIndices queue_family = littlevk::find_queue_families(phdev, surface);

	auto memory_properties = phdev.getMemoryProperties();
	auto device = littlevk::device(phdev, queue_family, EXTENSIONS);
	auto dal = littlevk::Deallocator(device);

	auto allocator = littlevk::bind(device, memory_properties, dal);
	auto combined = littlevk::bind(phdev, device);

	auto swapchain = combined.swapchain(surface, queue_family);

	auto graphics_queue = device.getQueue(queue_family.graphics, 0);
	auto present_queue = device.getQueue(queue_family.present, 0);

	vk::RenderPass render_pass = littlevk::RenderPassAssembler(device, dal)
		.add_attachment(littlevk::default_color_attachment(swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

	littlevk::Image depth_buffer = allocator
		.image(window.extent,
			vk::Format::eD32Sfloat,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::ImageAspectFlagBits::eDepth);

	littlevk::FramebufferGenerator generator(device, render_pass, window.extent, dal);
	for (const auto &view : swapchain.image_views)
		generator.add(view, depth_buffer.view);

	auto framebuffers = generator.unpack();

	auto command_pool = littlevk::command_pool(device,
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queue_family.graphics).unwrap(dal);

	auto command_buffers = littlevk::command_buffers(device,
		command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);

	std::vector <jvl::gfx::VulkanTriangleMesh> vmeshes;
	for (size_t i = 0; i < preset.geometry.size(); i++) {
		auto tmesh = jvl::core::TriangleMesh::from(preset.geometry[i]).value();
		auto vmesh = jvl::gfx::VulkanTriangleMesh::from(allocator, tmesh).value();
		vmeshes.push_back(vmesh);
	}

	// Create a graphics pipeline
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

	auto bundle = littlevk::ShaderStageBundle(device, dal)
		.source(vertex_shader_source, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader_source, vk::ShaderStageFlagBits::eFragment);

	littlevk::Pipeline ppl = littlevk::PipelineAssembler <littlevk::eGraphics> (device, window, dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <MVP> (vk::ShaderStageFlagBits::eVertex, 0)
		.cull_mode(vk::CullModeFlagBits::eNone);

	// Syncronization primitives
	auto sync = littlevk::present_syncronization(device, 2).unwrap(dal);

	jvl::core::Aperature aperature;
	jvl::core::Transform transform;

	MVP mvp;
	mvp.model = jvl::float4x4::identity();
	mvp.proj = jvl::core::perspective(aperature);

	auto resize = [&]() {
		combined.resize(surface, window, swapchain);

		// Recreate the depth buffer
		littlevk::Image depth_buffer = allocator
			.image(window.extent,
				vk::Format::eD32Sfloat,
				vk::ImageUsageFlagBits::eDepthStencilAttachment,
				vk::ImageAspectFlagBits::eDepth);

		// We can use the same generator; unpack() clears previously made framebuffers
		generator.extent = window.extent;
		for (const auto &view : swapchain.image_views)
			generator.add(view, depth_buffer.view);

		framebuffers = generator.unpack();

		aperature.aspect = float(window.extent.width)/float(window.extent.height);
		mvp.proj = jvl::core::perspective(aperature);
	};

	auto handle_input = [&]() {
		static float last_time = 0.0f;

		constexpr float speed = 500.0f;

		float delta = speed * float(glfwGetTime() - last_time);
		last_time = glfwGetTime();

		jvl::float3 velocity(0.0f);
		if (glfwGetKey(window.handle, GLFW_KEY_S) == GLFW_PRESS)
			velocity.z -= delta;
		else if (glfwGetKey(window.handle, GLFW_KEY_W) == GLFW_PRESS)
			velocity.z += delta;

		if (glfwGetKey(window.handle, GLFW_KEY_D) == GLFW_PRESS)
			velocity.x -= delta;
		else if (glfwGetKey(window.handle, GLFW_KEY_A) == GLFW_PRESS)
			velocity.x += delta;

		if (glfwGetKey(window.handle, GLFW_KEY_E) == GLFW_PRESS)
			velocity.y += delta;
		else if (glfwGetKey(window.handle, GLFW_KEY_Q) == GLFW_PRESS)
			velocity.y -= delta;

		transform.translate += transform.rotation.rotate(velocity);
	};

	glfwSetWindowUserPointer(window.handle, &transform);
	glfwSetMouseButtonCallback(window.handle, button_callback);
	glfwSetCursorPosCallback(window.handle, cursor_callback);

	uint32_t frame = 0;
        while (true) {
		handle_input();

		mvp.view = transform.to_view_matrix();

                glfwPollEvents();
                if (glfwWindowShouldClose(window.handle))
                        break;

		littlevk::SurfaceOperation op;
                op = littlevk::acquire_image(device, swapchain.swapchain, sync[frame]);
		if (op.status == littlevk::SurfaceOperation::eResize) {
			resize();
			continue;
		}

		// Start the render pass
		const auto &cmd = command_buffers[frame];
		cmd.begin(vk::CommandBufferBeginInfo {});

		// Set viewport and scissor
		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(window));

		const auto &rpbi = littlevk::default_rp_begin_info <2>
			(render_pass, framebuffers[op.index], window);

		cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);

		// Render the triangle
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);

		cmd.pushConstants <MVP> (ppl.layout, vk::ShaderStageFlagBits::eVertex, 0, mvp);

		for (const auto &vmesh : vmeshes) {
			cmd.bindVertexBuffers(0, vmesh.vertices.buffer, { 0 });
			cmd.bindIndexBuffer(vmesh.triangles.buffer, 0, vk::IndexType::eUint32);
			cmd.drawIndexed(vmesh.count, 1, 0, 0, 0);
		}

		cmd.endRenderPass();
		cmd.end();

		// Submit command buffer while signaling the semaphore
		constexpr vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		vk::SubmitInfo submit_info {
			sync.image_available[frame],
			wait_stage, cmd,
			sync.render_finished[frame]
		};

		graphics_queue.submit(submit_info, sync.in_flight[frame]);

                op = littlevk::present_image(present_queue, swapchain.swapchain, sync[frame], op.index);
		if (op.status == littlevk::SurfaceOperation::eResize)
			resize();

		frame = 1 - frame;
        }

	device.waitIdle();

	window.drop();

	dal.drop();
}
