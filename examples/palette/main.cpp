#include <core/aperature.hpp>
#include <core/color.hpp>
#include <core/scene.hpp>
#include <core/transform.hpp>
#include <engine/device_resource_collection.hpp>
#include <engine/imported_asset.hpp>
#include <gfx/cpu/scene.hpp>
#include <gfx/vulkan/scene.hpp>
#include <ire/core.hpp>

using namespace jvl;
using namespace jvl::ire;

// Palette generation from SV values
template <size_t N>
array <vec3, N> hsv_palette(float saturation, float lightness)
{
	std::array <vec3, N> palette;

	float step = 360.0f / float(N);
	for (int32_t i = 0; i < N; i++) {
		float3 hsv = float3(i * step, saturation, lightness);
		float3 rgb = core::hsv_to_rgb(hsv);
		palette[i] = vec3(rgb.x, rgb.y, rgb.z);
	}

	return palette;
}

// Model-view-projection structure
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() const {
		return uniform_layout(
			"MVP",
			named_field(model),
			named_field(view),
			named_field(proj)
		);
	}
};

// Shader kernels
void vertex()
{
	// Vertex inputs
	layout_in <vec3> position(0);
	
	// Object triangle index
	layout_out <int, flat> out_id(0);
	
	// Projection information
	push_constant <MVP> mvp;

	// Projecting the vertex
	gl_Position = mvp.project(position);

	// Transfering triangle index
	out_id = (gl_VertexIndex / 3);
}

// TODO: pass hsv
void fragment()
{
	// Triangle index
	layout_in <int, flat> id(0);
	
	// Resulting fragment color
	layout_out <vec4> fragment(0);
		
	auto palette = hsv_palette <16> (0.5, 1);

	fragment = vec4(palette[id % palette.size()], 1);
}

// Constructing the graphics pipeline
littlevk::Pipeline configure_pipeline(engine::DeviceResourceCollection &drc, const vk::RenderPass &render_pass)
{
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

	auto vs_callable = callable("main") << vertex;
	auto fs_callable = callable("main") << fragment;

	std::string vertex_shader = link(vs_callable).generate_glsl();
	std::string fragment_shader = link(fs_callable).generate_glsl();

	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	return littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t <MVP>))
		.cull_mode(vk::CullModeFlagBits::eNone);
}

void handle_input(const engine::InteractiveWindow &window, core::Transform &camera_transform)
{
	static constexpr float speed = 100.0f;
	static float last_time = 0.0f;

	float delta = speed * float(glfwGetTime() - last_time);
	last_time = glfwGetTime();

	jvl::float3 velocity(0.0f);
	if (window.key_pressed(GLFW_KEY_S))
		velocity.z -= delta;
	else if (window.key_pressed(GLFW_KEY_W))
		velocity.z += delta;

	if (window.key_pressed(GLFW_KEY_D))
		velocity.x -= delta;
	else if (window.key_pressed(GLFW_KEY_A))
		velocity.x += delta;

	if (window.key_pressed(GLFW_KEY_E))
		velocity.y -= delta;
	else if (window.key_pressed(GLFW_KEY_Q))
		velocity.y += delta;

	camera_transform.translate += camera_transform.rotation.rotate(velocity);
}
	
static bool voided = true;
static bool dragging = false;
static float last_x = 0;
static float last_y = 0;
static float pitch = 0;
static float yaw = 0;

static core::Transform *active_transform = nullptr;

void handle_cursor(float2 mouse, core::Transform &transform)
{
	static constexpr float sensitivity = 0.0025f;

	if (!dragging)
		return;

	if (voided) {
		last_x = mouse.x;
		last_y = mouse.y;
		voided = false;
	}

	double dx = mouse.x - last_x;
	double dy = mouse.y - last_y;

	// Dragging state
	pitch -= dx * sensitivity;
	yaw -= dy * sensitivity;

	float pi_e = pi <float> / 2.0f - 1e-3f;
	yaw = std::min(pi_e, std::max(-pi_e, yaw));

	last_x = mouse.x;
	last_y = mouse.y;

	transform.rotation = fquat::euler_angles(yaw, pitch, 0);
}

void glfw_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			dragging = true;
		} else if (action == GLFW_RELEASE) {
			dragging = false;
			voided = true;
		}
	}
}

void glfw_cursor_callback(GLFWwindow *window, double x, double y)
{
	if (!active_transform)
		return;

	handle_cursor(float2(x, y), *active_transform);
}

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	// littlevk configuration
	{
		auto &config = littlevk::config();
		config.enable_logging = false;
		config.abort_on_validation_error = true;
	}
	
	// Load the asset and scene
	std::filesystem::path path = argv[1];

	// Device extensions
	static const std::vector <const char *> EXTENSIONS {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	// Load physical device
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, EXTENSIONS);
	};

	// Configure the resource collection
	engine::DeviceResourceCollectionInfo info {
		.phdev = littlevk::pick_physical_device(predicate),
		.title = "Editor",
		.extent = vk::Extent2D(1920, 1080),
		.extensions = EXTENSIONS,
	};
	
	auto drc = engine::DeviceResourceCollection::from(info);

	// Load the scene
	auto asset = engine::ImportedAsset::from(path).value();
	auto scene = core::Scene();
	scene.add(asset);

	// Prepare host and device scenes
	auto host_scene = gfx::cpu::Scene::from(scene);
	auto vk_scene = gfx::vulkan::Scene::from(drc.allocator(), host_scene);

	// Create the render pass and generate the pipelines
	vk::RenderPass render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

	auto ppl = configure_pipeline(drc, render_pass);
	
	// Create the frame buffers
	auto &extent = drc.window.extent;

	littlevk::Image depth = drc.allocator()
		.image(extent,
			vk::Format::eD32Sfloat,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::ImageAspectFlagBits::eDepth);

	littlevk::FramebufferGenerator generator {
		drc.device, render_pass,
		extent, drc.dal
	};

	for (size_t i = 0; i < drc.swapchain.images.size(); i++)
		generator.add(drc.swapchain.image_views[i], depth.view);

	auto framebuffers = generator.unpack();

	// Camera transform and aperature
	core::Transform camera_transform;
	core::Transform model_transform;
	core::Aperature aperature;

	// MVP structure used for push constants
	auto m_model = uniform_field(MVP, model);
	auto m_view = uniform_field(MVP, view);
	auto m_proj = uniform_field(MVP, proj);
	
	solid_t <MVP> mvp;

	mvp[m_model] = model_transform.to_mat4();
	
	// Command buffers for the rendering loop
	auto command_buffers = littlevk::command_buffers(drc.device,
		drc.command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);
	
	// Synchronization information
	auto frame = 0u;
	auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);
	
	// Handling mouse events
	active_transform = &camera_transform;
	glfwSetMouseButtonCallback(drc.window.handle, glfw_button_callback);
	glfwSetCursorPosCallback(drc.window.handle, glfw_cursor_callback);

	// Main loop
	auto &window = drc.window;
	while (!window.should_close()) {
		window.poll();

		auto &cmd = command_buffers[frame];
		auto sync_frame = sync[frame];

		handle_input(drc.window, camera_transform);
	
		// Grab the next image
		littlevk::SurfaceOperation sop;
		sop = littlevk::acquire_image(drc.device, drc.swapchain.swapchain, sync_frame);
		if (sop.status == littlevk::SurfaceOperation::eResize) {
			drc.combined().resize(drc.surface, drc.window, drc.swapchain);
			continue;
		}
	
		// Start the command buffer
		vk::CommandBufferBeginInfo cbbi;
		cmd.begin(cbbi);
		
		// Configure the rendering extent
		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(drc.window.extent));

		auto rpbi = littlevk::default_rp_begin_info <2> (render_pass,
			framebuffers[sop.index],
			drc.window.extent);

		cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);
	
		// Update the constants with the view matrix
		auto &extent = drc.window.extent;
		aperature.aspect = float(extent.width)/float(extent.height);
		mvp[m_proj] = core::perspective(aperature);
		mvp[m_view] = camera_transform.to_view_matrix();
		
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.handle);
	
		for (auto &mesh : vk_scene.meshes) {
			int mid = *mesh.material_usage.begin();

			cmd.pushConstants <solid_t <MVP>> (ppl.layout,
				vk::ShaderStageFlagBits::eVertex,
				0, mvp);

			cmd.bindVertexBuffers(0, mesh.vertices.buffer, { 0 });
			cmd.bindIndexBuffer(mesh.triangles.buffer, 0, vk::IndexType::eUint32);
			cmd.drawIndexed(mesh.count, 1, 0, 0, 0);
		}

		cmd.endRenderPass();
		cmd.end();
	
		// Submit command buffer while signaling the semaphore
		constexpr vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		vk::SubmitInfo submit_info {
			sync_frame.image_available,
			wait_stage, cmd,
			sync_frame.render_finished
		};

		drc.graphics_queue.submit(submit_info, sync_frame.in_flight);
	
		// Present to the window
		sop = littlevk::present_image(drc.present_queue, drc.swapchain.swapchain, sync_frame, sop.index);
		if (sop.status == littlevk::SurfaceOperation::eResize)
			drc.combined().resize(drc.surface, drc.window, drc.swapchain);
	
		// Advance to the next frame
		frame = 1 - frame;
	}
}