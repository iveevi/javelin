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

#include "shaders.hpp"

struct Application : CameraApplication {
	littlevk::Pipeline raster;
	vk::RenderPass render_pass;
	DefaultFramebufferSet framebuffers;
	
	Transform model_transform;

	// TODO: argument for number of particles
	int N = 150'000;

	// TODO: solidify... (alignment issues)
	std::vector <glm::vec3> points = generate_random_points(N, 10.0f);
	std::vector <aligned_vec3> velocities;
	std::vector <aligned_vec3> particles;

	TriangleMesh sphere;

	littlevk::Buffer vb;
	littlevk::Buffer ib;
	littlevk::Buffer tb;
	littlevk::Buffer sb;

	vk::DescriptorSet raster_dset;

	std::string cmap = "jet";
	float dt = 0.02f;
	float mass = 500.0f;
	bool automatic;

	Application() : CameraApplication("Particles", { VK_KHR_SWAPCHAIN_EXTENSION_NAME })
	{
		render_pass = littlevk::RenderPassAssembler(resources.device, resources.dal)
			.add_attachment(littlevk::default_color_attachment(resources.swapchain.format))
			.add_attachment(littlevk::default_depth_attachment())
			.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

		framebuffers.resize(resources, render_pass);

		// Configure ImGui
		configure_imgui(resources, render_pass);

		configure_pipeline(jet);

		// Prepare the sphere geometry
		sphere = TriangleMesh::uv_sphere(25, 0.025f);

		std::tie(vb, ib) = resources.allocator()
			.buffer(sphere.positions,
				vk::BufferUsageFlagBits::eVertexBuffer
				| vk::BufferUsageFlagBits::eTransferDst)
			.buffer(sphere.triangles,
				vk::BufferUsageFlagBits::eIndexBuffer
				| vk::BufferUsageFlagBits::eTransferDst);
	}

	// Function to generate random points and use them as positions for spheres
	static std::vector <glm::vec3> generate_random_points(int N, float spread) {
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

	static void integrator(std::vector <aligned_vec3> &particles,
			       std::vector <aligned_vec3> &velocities,
			       float dt,
			       float M) {
		float t = glfwGetTime();

		static glm::vec3 O1 = { -5, 0, 0 };
		static glm::vec3 V1 = { -1, -2, 2 };
		
		static glm::vec3 O2 = { 5, 0, 0 };
		static glm::vec3 V2 = { 1, 2, -2 };

		glm::vec3 mid = 0.5f * (O1 + O2);
		float R = 20.0f + length(O1 - O2);

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
				float theta = glm::linearRand(-1.0, 1.0) * 2.0f * M_PI;
				float phi = std::acos(glm::linearRand(-1.0, 1.0));

				float x = std::sin(phi) * std::cos(theta);
				float y = std::sin(phi) * std::sin(theta);
				float z = std::cos(phi);
				float r = R * std::sqrt(glm::linearRand(0.0, 1.0));

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

	// Pipeline configuration for rendering spheres
	void configure_pipeline(vec3 (*cmap)(f32)) {
		auto vertex_layout = littlevk::VertexLayout <littlevk::rgb32f> ();

		auto vs_callable = procedure("main") << vertex;
		auto fs_callable = procedure("main") << std::make_tuple(cmap) << fragment;

		auto vs_spv = link(vs_callable).generate(Target::spirv_binary_via_glsl, Stage::vertex);
		auto fs_spv = link(fs_callable).generate(Target::spirv_binary_via_glsl, Stage::fragment);

		// TODO: automatic generation by observing used layouts
		auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
			.code(vs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eVertex)
			.code(fs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eFragment);

		shader_debug();

		raster = littlevk::PipelineAssembler <littlevk::PipelineType::eGraphics>
			(resources.device, resources.window, resources.dal)
			.with_render_pass(render_pass, 0)
			.with_vertex_layout(vertex_layout)
			.with_shader_bundle(bundle)
			.with_push_constant <solid_t <MVP>> (vk::ShaderStageFlagBits::eVertex, 0)
			.with_push_constant <solid_t <u32>> (vk::ShaderStageFlagBits::eFragment, sizeof(solid_t <MVP>))
			.with_dsl_binding(0, vk::DescriptorType::eStorageBuffer,
					1, vk::ShaderStageFlagBits::eVertex)
			.with_dsl_binding(1, vk::DescriptorType::eStorageBuffer,
					1, vk::ShaderStageFlagBits::eVertex)
			.cull_mode(vk::CullModeFlagBits::eNone);
	}

	void preload(const argparse::ArgumentParser &program) override {
		for (auto p : points) {
			particles.push_back(p);

			float theta = glm::linearRand(-1.0, 1.0) * 2.0f * M_PI;
			float phi = std::acos(glm::linearRand(-1.0, 1.0));

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
		
		automatic = (program["auto"] == true);
	}
	
	void render(const vk::CommandBuffer &cmd, uint32_t index, uint32_t) override {
		if (automatic) {
			glm::vec3 min = glm::vec3(1e10);
			glm::vec3 max = -min;

			for (auto &p4 : particles) {
				auto p = glm::vec3(p4);
				min = glm::min(min, p);
				max = glm::max(max, p);
			}

			auto &xform = camera.transform;

			float angle = 0;
			float r = 0.75 * glm::length(max - min);
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

		for (const auto &v : velocities) {
			float s = length(glm::vec3(v));
			smin = std::min(smin, s);
			smax = std::max(smax, s);
		}

		view_info.get <2> () = smin;
		view_info.get <3> () = smax;

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

		// Randomly jittering particles
		integrator(particles, velocities, dt, mass);

		littlevk::upload(resources.device, tb, particles);
		littlevk::upload(resources.device, sb, velocities);
	}

	void render_imgui(const vk::CommandBuffer &cmd) {
		ImGuiRenderContext context(cmd);

		ImGui::Begin("Configure Simulation");
		{
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
						configure_pipeline(m);
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
	
	void resize() override {
		framebuffers.resize(resources, render_pass);
	}
};

APPLICATION_MAIN()