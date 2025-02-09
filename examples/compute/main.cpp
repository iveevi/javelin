#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include <argparse/argparse.hpp>

#include <ire/core.hpp>

#include "aperature.hpp"
#include "camera_controller.hpp"
#include "cmaps.hpp"
#include "default_framebuffer_set.hpp"
#include "device_resource_collection.hpp"
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

auto project_vertex = [](MVP &info, vec3 &position, vec3 vertex)
{
	// TODO: put the buffer here...
	vec4 p = vec4(vertex + position, 1);
	return info.proj * (info.view * p);
};

// Shader kernels for the sphere rendering
void vertex()
{
	layout_in <vec3> position(0);

	layout_out <vec3> out_position(0);
	layout_out <f32> out_speed(1);
	layout_out <vec2> out_range(2);

	read_only_buffer <unsized_array <vec3>> positions(0);
	read_only_buffer <unsized_array <vec3>> velocities(1);
	
	push_constant <MVP> view_info;

	vec3 translate = positions[gl_InstanceIndex];
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
std::vector <float3> generate_random_points(int N, float spread)
{
	std::vector <float3> points;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution <float> dis(-spread, spread);

	for (int i = 0; i < N; i++) {
		while (true) {
			float x = dis(gen);
			float y = dis(gen);
			float z = dis(gen);
			float3 p = float3(x, y, z);
			if (length(p) > spread / 2.0f) {
				points.push_back(p);
				break;
			}
		}
	}

	return points;
}

// Pipeline configuration for rendering spheres
littlevk::Pipeline configure_pipeline(core::DeviceResourceCollection &drc,
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
	auto controller = reinterpret_cast <engine::CameraController *> (glfwGetWindowUserPointer(window));

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

struct SimulationInfo {
	vec3 O1;
	vec3 O2;
	f32 M;
	f32 dt;

	auto layout() const {
		return uniform_layout("SimulationInfo",
			named_field(O1),
			named_field(O2),
			named_field(M),
			named_field(dt));
	}
};

auto integrator()
{
	local_size group(32);

	push_constant <SimulationInfo> info;
	
	buffer <unsized_array <vec3>> positions(0);
	buffer <unsized_array <vec3>> velocities(1);

	u32 tid = gl_GlobalInvocationID.x;

	vec3 p = positions[tid];
	vec3 v = velocities[tid];

	vec3 d;
	f32 l;

	d = (p - info.O1);	
	l = max(length(d), 1.0f);
	v = v - info.dt * info.M * d / pow(l, 3.0f);
	
	d = (p - info.O2);	
	l = max(length(d), 1.0f);
	v = v - info.dt * info.M * d / pow(l, 3.0f);

	p += info.dt * v;

	velocities[tid] = v;
	positions[tid] = p;
}

void glfw_cursor_callback(GLFWwindow *window, double x, double y)
{
	ImGuiIO &io = ImGui::GetIO();
	io.MousePos = ImVec2(x, y);

	auto controller = reinterpret_cast <engine::CameraController *> (glfwGetWindowUserPointer(window));
	controller->handle_cursor(float2(x, y));
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
		config.enable_logging = false;
		config.abort_on_validation_error = true;
	}

	// Load the asset and scene
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, VK_EXTENSIONS);
	};

	core::DeviceResourceCollectionInfo info{
		.phdev = littlevk::pick_physical_device(predicate),
			.title = "Point Cloud Renderer",
			.extent = vk::Extent2D(1920, 1080),
			.extensions = VK_EXTENSIONS,
	};

	auto drc = core::DeviceResourceCollection::from(info);

	// Create the render pass and generate the pipelines
	vk::RenderPass render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
		.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
		.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.done();

	// Configure ImGui
	engine::configure_imgui(drc, render_pass);

	// Generate random points for the point cloud
	int N = 200'000;

	// TODO: solidify... (alignment issues)
	std::vector <float3> points = generate_random_points(N, 10.0f);
	std::vector <aligned_float3> velocities;
	std::vector <aligned_float3> particles;
	
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

		float3 v = float3(x, y, z);
		
		velocities.push_back(v);
	}

	// Prepare the sphere geometry
	auto sphere = core::TriangleMesh::uv_sphere(25, 0.025f);

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

	auto tb_info = vk::DescriptorBufferInfo()
		.setBuffer(tb.buffer)
		.setOffset(0)
		.setRange(sizeof(aligned_float3) * points.size());
	
	auto sb_info = vk::DescriptorBufferInfo()
		.setBuffer(sb.buffer)
		.setOffset(0)
		.setRange(sizeof(aligned_float3) * points.size());

	std::array <vk::WriteDescriptorSet, 2> writes;

	writes[0] = vk::WriteDescriptorSet()
		.setDescriptorCount(1)
		.setDstBinding(0)
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setBufferInfo(tb_info);
	
	writes[1] = vk::WriteDescriptorSet()
		.setDescriptorCount(1)
		.setDstBinding(1)
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setBufferInfo(sb_info);

	// Configure pipeline
	auto pipeline = configure_pipeline(drc, render_pass, jet);

	vk::DescriptorSet set = littlevk::bind(drc.device, drc.descriptor_pool)
		.allocate_descriptor_sets(pipeline.dsl.value())
		.front();

	writes[0].setDstSet(set);
	writes[1].setDstSet(set);

	drc.device.updateDescriptorSets(writes, {}, {});
	
	// Integration pipeline
	auto cs = procedure("main") << integrator;
	auto compute_shader = link(cs).generate_glsl();

	fmt::println("{}", compute_shader);

	auto bundle = littlevk::ShaderStageBundle(drc.device, drc.dal)
		.source(compute_shader, vk::ShaderStageFlagBits::eCompute);

	littlevk::Pipeline compute = littlevk::PipelineAssembler <littlevk::eCompute> (drc.device, drc.dal)
		.with_shader_bundle(bundle)
		.with_push_constant <SimulationInfo> (vk::ShaderStageFlagBits::eCompute)
		.with_dsl_binding(0, vk::DescriptorType::eStorageBuffer,
				  1, vk::ShaderStageFlagBits::eCompute)
		.with_dsl_binding(1, vk::DescriptorType::eStorageBuffer,
				  1, vk::ShaderStageFlagBits::eCompute);
	
	vk::DescriptorSet compute_set = littlevk::bind(drc.device, drc.descriptor_pool)
		.allocate_descriptor_sets(compute.dsl.value())
		.front();
	
	writes[0].setDstSet(compute_set);
	writes[1].setDstSet(compute_set);
	
	drc.device.updateDescriptorSets(writes, {}, {});

	// Framebuffer manager
	DefaultFramebufferSet framebuffers;
	framebuffers.resize(drc, render_pass);

	// Camera transform and aperture
	core::Transform camera_transform;
	core::Aperature aperature;

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
	bool pause = false;

	// Handling camera events
	engine::CameraController controller {
		camera_transform,
		engine::CameraControllerSettings()
	};

	glfwSetWindowUserPointer(drc.window.handle, &controller);
	glfwSetMouseButtonCallback(drc.window.handle, glfw_button_callback);
	glfwSetCursorPosCallback(drc.window.handle, glfw_cursor_callback);

	// Handling window resizing
	auto resize = [&]() { framebuffers.resize(drc, render_pass); };

	// Gravity points
	static float3 O1 = float3 { -5, 0, 0 };
	static float3 V1 = float3 { -1, -2, 2 };
	
	static float3 O2 = float3 { 5, 0, 0 };
	static float3 V2 = float3 { 1, 2, -2 };

	auto render = [&](const vk::CommandBuffer &cmd, uint32_t index) {
		static bool down = false;

		if (drc.window.key_pressed(GLFW_KEY_SPACE)) {
			if (!down) {
				pause = !pause;
				down = true;
			}
		} else {
			down = false;
		}

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

		auto proj = perspective(aperature);
		auto view = camera_transform.to_view_matrix();

		view_info[m_proj] = proj;
		view_info[m_view] = view;

		float smin = 1e10f;
		float smax = 0;

		littlevk::download(drc.device, sb, velocities);

		for (const auto &v : velocities) {
			float s = length(v);
			smin = std::min(smin, s);
			smax = std::max(smax, s);
		}

		if (fabs(smax - smin) < 1e-3f) {
			smax += 1e-3f;
			smin -= 1e-3f;
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
			engine::ImGuiRenderContext context(cmd);

			ImGui::Begin("Simulation Info");

			ImGui::Separator();
			ImGui::Text("Framerate: %3.2f", ImGui::GetIO().Framerate);

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

		// Integration pass
		if (!pause) {
			solid_t <SimulationInfo> info;
			info.get <0> () = O1;
			info.get <1> () = O2;
			info.get <2> () = mass;
			info.get <3> () = dt;

			cmd.bindPipeline(vk::PipelineBindPoint::eCompute, compute.handle);

			cmd.pushConstants <solid_t <SimulationInfo>> (compute.layout,
				vk::ShaderStageFlagBits::eCompute,
				0, info);

			cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
				compute.layout,
				0, compute_set, {});

			cmd.dispatch((N + 31) / 32, 1, 1);
		}

		// Update primary particles
		float3 mid = 0.5f * (O1 + O2);
		float R = 20.0f + length(O1 - O2);

		float3 D = (O1 - O2);
		float L = length(D);
		float3 A = mass * mass * D / powf(L, 3.0f);

		V1 -= dt * A / mass;
		V2 += dt * A / mass;

		O1 += dt * V1;
		O2 += dt * V2;
	};

	drc.swapchain_render_loop(timed(drc.window, render, program.get <double> ("limit")), resize);

	return 0;
}
