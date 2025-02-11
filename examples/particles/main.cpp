#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <argparse/argparse.hpp>

#include <ire/core.hpp>

#include "aperature.hpp"
#include "camera_controller.hpp"
#include "cmaps.hpp"
#include "default_framebuffer_set.hpp"
#include "vulkan_resources.hpp"
#include "extensions.hpp"
#include "imgui.hpp"
#include "timer.hpp"
#include "transform.hpp"
#include "triangle_mesh.hpp"

#include <random>

using namespace jvl;
using namespace jvl::ire;

VULKAN_EXTENSIONS(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

// View and projection information
struct MVP {
	mat4 view;
	mat4 proj;
	f32 smin;
	f32 smax;

	auto layout() const {
		return uniform_layout(
			"ViewInfo",
			named_field(view),
			named_field(proj),
			named_field(smin),
			named_field(smax));
	}
};

auto project_vertex = [](MVP &info, vec3 &translate, vec3 position)
{
	// TODO: put the buffer here...
	vec4 p = vec4(position + translate, 1);
	return info.proj * (info.view * p);
};

// Shader kernels for the sphere rendering
void vertex()
{
	layout_in <vec3> position(0);

	layout_out <vec3> out_position(0);
	layout_out <f32> out_speed(1);
	layout_out <vec2> out_range(2);

	read_only_buffer <unsized_array <vec3>> translations(0);
	read_only_buffer <unsized_array <vec3>> velocities(1);
	
	push_constant <MVP> view_info;

	vec3 translate = translations[gl_InstanceIndex];
	vec3 velocity = velocities[gl_InstanceIndex];
	
	gl_Position = project_vertex(view_info, translate, position);

	out_position = position;
	out_speed = length(velocity);
	out_range = vec2(view_info.smin, view_info.smax);
}

void fragment(vec3 (*cmap)(f32))
{
	layout_in <vec3> position(0);
	layout_in <f32> speed(1);
	layout_in <vec2> range(2);

	layout_out <vec4> fragment(0);

	f32 t = (speed - range.x) / (range.y - range.x);

	fragment = vec4(cmap(t), 1.0f);
}

// Function to generate random points and use them as positions for spheres
std::vector <glm::vec3> generate_random_points(int N, float spread)
{
	std::vector <glm::vec3> points;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution <float> dis(-spread, spread);

	for (int i = 0; i < N; i++) {
		while (true) {
			float x = dis(gen);
			float y = dis(gen);
			float z = dis(gen);
			glm::vec3 p = glm::vec3(x, y, z);
			if (length(p) > spread / 2.0f) {
				points.push_back(p);
				break;
			}
		}
	}

	return points;
}

// Pipeline configuration for rendering spheres
littlevk::Pipeline configure_pipeline(VulkanResources &drc,
				      const vk::RenderPass &render_pass,
				      auto cmap)
{
	auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

	auto vs_callable = procedure("main") << vertex;
	auto fs_callable = procedure("main") << std::make_tuple(cmap) << fragment;

	std::string vertex_shader = link(vs_callable).generate_glsl();
	std::string fragment_shader = link(fs_callable).generate_glsl();

	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(vertex_shader, vk::ShaderStageFlagBits::eVertex)
		.source(fragment_shader, vk::ShaderStageFlagBits::eFragment);

	fmt::println("{}", vertex_shader);
	fmt::println("{}", fragment_shader);

	return littlevk::PipelineAssembler <littlevk::eGraphics> (drc.device, drc.window, drc.dal)
		.with_render_pass(render_pass, 0)
		.with_vertex_layout(vertex_layout)
		.with_shader_bundle(bundle)
		.with_push_constant <solid_t<MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
		.with_push_constant <solid_t<u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t<MVP>))
		.with_dsl_binding(0, vk::DescriptorType::eStorageBuffer,
				  1, vk::ShaderStageFlagBits::eVertex)
		.with_dsl_binding(1, vk::DescriptorType::eStorageBuffer,
				  1, vk::ShaderStageFlagBits::eVertex)
		.cull_mode(vk::CullModeFlagBits::eNone);
}

void glfw_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	auto controller = reinterpret_cast <CameraController *> (glfwGetWindowUserPointer(window));

	ImGuiIO &io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		io.AddMouseButtonEvent(button, action);
		controller->voided = true;
		controller->dragging = false;
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			controller->dragging = true;
		} else if (action == GLFW_RELEASE) {
			controller->dragging = false;
			controller->voided = true;
		}
	}
}

void glfw_cursor_callback(GLFWwindow *window, double x, double y)
{
	ImGuiIO &io = ImGui::GetIO();
	io.MousePos = ImVec2(x, y);

	auto controller = reinterpret_cast <CameraController *> (glfwGetWindowUserPointer(window));
	controller->handle_cursor(glm::vec2(x, y));
}

void jitter(std::vector <aligned_vec3> &particles,
	    std::vector <aligned_vec3> &velocities,
	    float dt,
	    float M)
{
	float t = glfwGetTime();

	static glm::vec3 O1 = { -5, 0, 0 };
	static glm::vec3 V1 = { -1, -2, 2 };
	
	static glm::vec3 O2 = { 5, 0, 0 };
	static glm::vec3 V2 = { 1, 2, -2 };

	glm::vec3 mid = 0.5f * (O1 + O2);
	float R = 20.0f + length(O1 - O2);
	
	std::random_device rd;
	std::default_random_engine gen(rd());
	std::uniform_real_distribution <float> unit(0.0f, 1.0f);
	std::uniform_real_distribution <float> distribution(-1.0f, 1.0f);

	#pragma omp parallel for
	for (size_t i = 0; i < particles.size(); ++i) {
		aligned_vec3 &v = velocities[i];
		aligned_vec3 &p = particles[i];

		{
			glm::vec3 d = (p - O1);	
			float l = fmax(1, length(d));
			v = v - dt * M * d / powf(l, 3.0f);
		}
		
		{
			glm::vec3 d = (p - O2);	
			float l = fmax(1, length(d));
			v = v - dt * M * d / powf(l, 3.0f);
		}

		p += dt * v;

		// Bounding
		if (length(p - mid) > R) {
			float theta = distribution(gen) * 2.0f * M_PI;
			float phi = std::acos(distribution(gen));

			float x = std::sin(phi) * std::cos(theta);
			float y = std::sin(phi) * std::sin(theta);
			float z = std::cos(phi);
			float r = R * std::sqrt(unit(gen));

			p = r * glm::vec3(x, y, z) + mid;
		}
	}

	glm::vec3 D = (O1 - O2);
	float L = length(D);
	glm::vec3 A = M * M * D / powf(L, 3.0f);

	V1 -= dt * A / M;
	V2 += dt * A / M;

	O1 += dt * V1;
	O2 += dt * V2;
}

int main(int argc, char *argv[])
{
	argparse::ArgumentParser program("particles");

	program.add_argument("--limit")
		.help("time limit (ms) for the execution of the program )")
		.default_value(-1.0)
		.scan <'g', double> ();

	try {
		program.parse_args(argc, argv);
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << program;
		return 1;
	}

        // Initialize Vulkan configuration
	{
		auto &config = littlevk::config();
		config.enable_logging = true;
		config.abort_on_validation_error = true;
	}

	auto drc = VulkanResources::from("Particles", vk::Extent2D(1920, 1080), VK_EXTENSIONS);

	// Create the render pass and generate the pipelines
	vk::RenderPass render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
		.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
		.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.done();

	// Configure ImGui
	configure_imgui(drc, render_pass);

	// Generate random points for the point cloud
	int N = 150'000;

	// TODO: solidify... (alignment issues)
	std::vector <glm::vec3> points = generate_random_points(N, 10.0f);
	std::vector <aligned_vec3> velocities;
	std::vector <aligned_vec3> particles;
	
	std::random_device rd;
	std::default_random_engine gen(rd());

	for (auto p : points) {
		particles.push_back(p);

		std::uniform_real_distribution <float> distribution(-1.0f, 1.0f);

		float theta = distribution(gen) * 2.0f * M_PI;
		float phi = std::acos(distribution(gen));

		float x = std::sin(phi) * std::cos(theta);
		float y = std::sin(phi) * std::sin(theta);
		float z = std::cos(phi);
		
		velocities.emplace_back(x, y, z);
	}

	// Prepare the sphere geometry
	auto sphere = TriangleMesh::uv_sphere(25, 0.025f);

	littlevk::Buffer vb;
	littlevk::Buffer ib;
	littlevk::Buffer tb;
	littlevk::Buffer sb;
	std::tie(vb, ib, tb, sb) = drc.allocator()
		.buffer(sphere.positions,
			vk::BufferUsageFlagBits::eVertexBuffer
			| vk::BufferUsageFlagBits::eTransferDst)
		.buffer(sphere.triangles,
			vk::BufferUsageFlagBits::eIndexBuffer
			| vk::BufferUsageFlagBits::eTransferDst)
		.buffer(particles,
			vk::BufferUsageFlagBits::eStorageBuffer
			| vk::BufferUsageFlagBits::eTransferDst)
		.buffer(velocities,
			vk::BufferUsageFlagBits::eStorageBuffer
			| vk::BufferUsageFlagBits::eTransferDst);

	// Configure pipeline
	auto pipeline = configure_pipeline(drc, render_pass, jet);

	vk::DescriptorSet set = littlevk::bind(drc.device, drc.descriptor_pool)
		.allocate_descriptor_sets(pipeline.dsl.value())
		.front();

	auto tb_info = vk::DescriptorBufferInfo()
		.setBuffer(tb.buffer)
		.setOffset(0)
		.setRange(sizeof(glm::vec4) * points.size());
	
	auto sb_info = vk::DescriptorBufferInfo()
		.setBuffer(sb.buffer)
		.setOffset(0)
		.setRange(sizeof(glm::vec4) * points.size());

	std::array <vk::WriteDescriptorSet, 2> writes;

	writes[0] = vk::WriteDescriptorSet()
		.setDstSet(set)
		.setDescriptorCount(1)
		.setDstBinding(0)
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setBufferInfo(tb_info);
	
	writes[1] = vk::WriteDescriptorSet()
		.setDstSet(set)
		.setDescriptorCount(1)
		.setDstBinding(1)
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setBufferInfo(sb_info);

	drc.device.updateDescriptorSets(writes, {}, {});

	// Framebuffer manager
	DefaultFramebufferSet framebuffers;
	framebuffers.resize(drc, render_pass);

	// Camera transform and aperture
	Transform camera_transform;
	Aperature aperature;

	// MVP structure used for push constants
	auto m_view = uniform_field(MVP, view);
	auto m_proj = uniform_field(MVP, proj);

	solid_t <MVP> view_info;

	// Command buffers for the rendering loop
	auto command_buffers = littlevk::command_buffers(drc.device,
		drc.command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);

	// Synchronization information
	auto frame = 0u;
	auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);

	// Simulation
	std::string cmap = "jet";
	float dt = 0.02f;
	float mass = 500.0f;

	// Handling camera events
	CameraController controller {
		camera_transform,
		CameraControllerSettings()
	};

	glfwSetWindowUserPointer(drc.window.handle, &controller);
	glfwSetMouseButtonCallback(drc.window.handle, glfw_button_callback);
	glfwSetCursorPosCallback(drc.window.handle, glfw_cursor_callback);

	// Handling window resizing
	auto resize = [&]() { framebuffers.resize(drc, render_pass); };

	Timer timer;
	timer.reset();

	double limit = program.get <double> ("limit");
	
	auto render = [&](const vk::CommandBuffer &cmd, uint32_t index) {
		if (limit >= 0 && timer.click() > limit)
			glfwSetWindowShouldClose(drc.window.handle, true);

		controller.handle_movement(drc.window);
		
		// Configure the rendering extent
		vk::Extent2D extent = drc.window.extent;

		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(extent));

		littlevk::RenderPassBeginInfo(2)
			.with_render_pass(render_pass)
			.with_framebuffer(framebuffers[index])
			.with_extent(extent)
			.clear_color(0, std::array <float, 4> { 0, 0, 0, 1 })
			.clear_depth(1, 1)
			.begin(cmd);

		// Update the constants with the view matrix
		aperature.aspect = float(extent.width) / float(extent.height);

		auto proj = aperature.perspective();
		auto view = camera_transform.view_matrix();

		view_info[m_proj] = proj;
		view_info[m_view] = view;

		float smin = 1e10f;
		float smax = 0;

		for (const auto &v : velocities) {
			float s = length(glm::vec3(v));
			smin = std::min(smin, s);
			smax = std::max(smax, s);
		}

		view_info.get <2> () = smin;
		view_info.get <3> () = smax;

		// Render each point as a sphere
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			pipeline.layout,
			0, set, {});
		
		cmd.pushConstants <solid_t <MVP>> (pipeline.layout,
			vk::ShaderStageFlagBits::eVertex,
			0, view_info);
		
		cmd.bindVertexBuffers(0, vb.buffer, { 0 });
		cmd.bindIndexBuffer(ib.buffer, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(3 * sphere.triangles.size(), N, 0, 0, 0);

		// ImGui
		{
			ImGuiRenderContext context(cmd);

			ImGui::Begin("Configure Simulation");

			static const std::map <std::string, vec3 (*)(f32)> maps {
				{ "cividis",	cividis  },
				{ "coolwarm",	coolwarm },
				{ "inferno",	inferno  },
				{ "jet",	jet      },
				{ "magma",	magma    },
				{ "parula",	parula   },
				{ "plasma",	plasma	 },
				{ "spectral",	spectral },
				{ "turbo",	turbo    },
				{ "viridis",	viridis  },
			};

			if (ImGui::BeginCombo("Color Map", cmap.c_str())) {
				for (auto &[k, m] : maps) {
					if (ImGui::Selectable(k.c_str(), k == cmap)) {
						pipeline = configure_pipeline(drc, render_pass, m);
						cmap = k;
					}
				}

				ImGui::EndCombo();
			}

			ImGui::SliderFloat("Time Step", &dt, 0.01, 0.1);
			ImGui::SliderFloat("Planetary Mass", &mass, 100, 1000);

			ImGui::End();
		}

		cmd.endRenderPass();

		// Randomly jittering particles
		jitter(particles, velocities, dt, mass);

		littlevk::upload(drc.device, tb, particles);
		littlevk::upload(drc.device, sb, velocities);
	};
	
	drc.swapchain_render_loop(timed(drc.window, render, program.get <double> ("limit")), resize);

	return 0;
}
