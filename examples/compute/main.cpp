#include <glm/gtc/random.hpp>

#include "common/aperature.hpp"
#include "common/camera_controller.hpp"
#include "common/cmaps.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/vulkan_resources.hpp"
#include "common/imgui.hpp"
#include "common/transform.hpp"
#include "common/triangle_mesh.hpp"
#include "common/application.hpp"

#include "app.hpp"

Application::Application() : CameraApplication("Particles", { VK_KHR_SWAPCHAIN_EXTENSION_NAME })
{
	render_pass = littlevk::RenderPassAssembler(resources.device, resources.dal)
		.add_attachment(littlevk::default_color_attachment(resources.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
		.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
		.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
		.done();

	framebuffers.resize(resources, render_pass);

	// Pipelines
	configure_render_pipeline(jet);
	configure_compute_pipeline();
	shader_debug();

	// Configure ImGui
	configure_imgui(resources, render_pass);

	// Prepare the sphere geometry
	sphere = TriangleMesh::uv_sphere(12, 0.025f);

	std::tie(vb, ib) = resources.allocator()
		.buffer(sphere.positions,
			vk::BufferUsageFlagBits::eVertexBuffer
			| vk::BufferUsageFlagBits::eTransferDst)
		.buffer(sphere.triangles,
			vk::BufferUsageFlagBits::eIndexBuffer
			| vk::BufferUsageFlagBits::eTransferDst);
}

Application::~Application()
{
	shutdown_imgui();
}

void Application::preload(const argparse::ArgumentParser &program)
{
	for (auto p : points) {
		particles.push_back(p);

		float theta = glm::linearRand(0, 1) * 2.0f * M_PI;
		float phi = std::acos(glm::linearRand(0, 1));

		float x = std::sin(phi) * std::cos(theta);
		float y = std::sin(phi) * std::sin(theta);
		float z = std::cos(phi);

		velocities.emplace_back(x, y, z);
	}

	std::tie(tb, sb) = resources.allocator()
		.buffer(particles,
			vk::BufferUsageFlagBits::eStorageBuffer
			| vk::BufferUsageFlagBits::eTransferDst)
		.buffer(velocities,
			vk::BufferUsageFlagBits::eStorageBuffer
			| vk::BufferUsageFlagBits::eTransferDst);

	// Bind for rasterization
	raster_dset = littlevk::bind(resources.device, resources.descriptor_pool)
		.allocate_descriptor_sets(raster.dsl.value())
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
		.setDstSet(raster_dset)
		.setDescriptorCount(1)
		.setDstBinding(0)
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setBufferInfo(tb_info);

	writes[1] = vk::WriteDescriptorSet()
		.setDstSet(raster_dset)
		.setDescriptorCount(1)
		.setDstBinding(1)
		.setDescriptorType(vk::DescriptorType::eStorageBuffer)
		.setBufferInfo(sb_info);

	resources.device.updateDescriptorSets(writes, {}, {});

	// Bind for compute
	compute_dset = littlevk::bind(resources.device, resources.descriptor_pool)
		.allocate_descriptor_sets(compute.dsl.value())
		.front();

	writes[0].setDstSet(compute_dset);
	writes[1].setDstSet(compute_dset);

	resources.device.updateDescriptorSets(writes, {}, {});

	automatic = (program["auto"] == true);
}

void Application::render(const vk::CommandBuffer &cmd, uint32_t index, uint32_t)
{
	if (automatic) {
		static std::vector <glm::vec4> buffer;
		if (buffer.size() < N)
			buffer.resize(N);

		littlevk::download(resources.device, tb, buffer);

		glm::vec3 min = glm::vec3(1e10);
		glm::vec3 max = -min;

		for (auto &p4 : buffer) {
			auto p = glm::vec3(p4);
			min = glm::min(min, p);
			max = glm::max(max, p);
		}

		auto &xform = camera.transform;

		float angle = 0;
		float r = std::min(50.0f, 0.75f * glm::length(max - min));
		float a = angle + glm::pi <float> () / 2.0f;

		xform.translate = glm::vec3 {
			r * glm::cos(angle),
			0,
			r * glm::sin(angle),
		};

		xform.rotation = glm::angleAxis(-a, glm::vec3(0, 1, 0));
	} else {
		camera.controller.handle_movement(resources.window);
	}

	// Configure the rendering extent
	vk::Extent2D extent = resources.window.extent;

	littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(extent));

	littlevk::RenderPassBeginInfo(2)
		.with_render_pass(render_pass)
		.with_framebuffer(framebuffers[index])
		.with_extent(extent)
		.clear_color(0, std::array <float, 4> { 0, 0, 0, 1 })
		.clear_depth(1, 1)
		.begin(cmd);

	// Update the constants with the view matrix
	solid_t <MVP> view_info;

	camera.aperature.aspect = float(extent.width) / float(extent.height);

	auto proj = camera.aperature.perspective();
	auto view = camera.transform.view_matrix();

	view_info.get <0> () = view;
	view_info.get <1> () = proj;

	float smin = 1e10f;
	float smax = 0;

	littlevk::download(resources.device, sb, velocities);

	for (const auto &v : velocities) {
		float s = length(glm::vec3(v));
		smin = std::min(smin, s);
		smax = std::max(smax, s);
	}

	if (fabs(smax - smin) < 1e-3f) {
		smax += 1e-3f;
		smin -= 1e-3f;
	}

	view_info.get <2> () = smin;
	view_info.get <3> () = smax;

	// Gravity points
	static glm::vec3 O1 = glm::vec3 { -5, 0, 0 };
	static glm::vec3 V1 = glm::vec3 { -1, -2, 2 };

	static glm::vec3 O2 = glm::vec3 { 5, 0, 0 };
	static glm::vec3 V2 = glm::vec3 { 1, 2, -2 };

	// Render each point as a sphere
	cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, raster.handle);

	cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
		raster.layout,
		0, raster_dset, {});

	cmd.pushConstants <solid_t <MVP>> (raster.layout,
		vk::ShaderStageFlagBits::eVertex,
		0, view_info);

	cmd.bindVertexBuffers(0, vb.buffer, { 0 });
	cmd.bindIndexBuffer(ib.buffer, 0, vk::IndexType::eUint32);
	cmd.drawIndexed(3 * sphere.triangles.size(), N, 0, 0, 0);

	render_imgui(cmd);

	cmd.endRenderPass();

	// Integration pass
	if (!pause) {
		solid_t <SimulationInfo> info;
		info.get <0> () = O1;
		info.get <1> () = O2;
		info.get <2> () = mass;
		info.get <3> () = dt;
		info.get <4> () = N;

		cmd.bindPipeline(vk::PipelineBindPoint::eCompute, compute.handle);

		cmd.pushConstants <solid_t <SimulationInfo>> (compute.layout,
			vk::ShaderStageFlagBits::eCompute,
			0, info);

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
			compute.layout,
			0, compute_dset, {});

		cmd.dispatch((N + 31) / 32, 1, 1);
	}

	// Update primary particles
	glm::vec3 mid = 0.5f * (O1 + O2);
	float R = 20.0f + length(O1 - O2);

	glm::vec3 D = (O1 - O2);
	float L = length(D);
	glm::vec3 A = mass * mass * D / powf(L, 3.0f);

	V1 -= dt * A / mass;
	V2 += dt * A / mass;

	O1 += dt * V1;
	O2 += dt * V2;
}

void Application::render_imgui(const vk::CommandBuffer &cmd)
{
	ImGuiRenderContext context(cmd);

	ImGui::Begin("Simulation Info");
	{
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
					configure_render_pipeline(m);
					cmap = k;
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SliderFloat("Time Step", &dt, 0.01, 0.1);
		ImGui::SliderFloat("Planetary Mass", &mass, 100, 1000);
	}
	ImGui::End();
}

void Application::resize()
{
	framebuffers.resize(resources, render_pass);
}

// Function to generate random points and use them as positions for spheres
std::vector <glm::vec3> Application::generate_random_points(int N, float spread)
{
	std::vector <glm::vec3> points;

	for (int i = 0; i < N; i++) {
		while (true) {
			glm::vec3 p = glm::linearRand(-glm::vec3(spread), glm::vec3(spread));
			if (length(p) > spread / 2.0f) {
				points.push_back(p);
				break;
			}
		}
	}

	return points;
}

APPLICATION_MAIN()
