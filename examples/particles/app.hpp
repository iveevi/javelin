#pragma once

#include <ire.hpp>

#include "common/aperature.hpp"
#include "common/camera_controller.hpp"
#include "common/cmaps.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/vulkan_resources.hpp"
#include "common/imgui.hpp"
#include "common/transform.hpp"
#include "common/triangle_mesh.hpp"
#include "common/application.hpp"

using namespace jvl;
using namespace jvl::ire;

struct alignas(16) aligned_vec3 : glm::vec3 {
	using glm::vec3::vec3;

	aligned_vec3(const glm::vec3 &v) : glm::vec3(v) {}
};

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

	Application();

	~Application();

	// Function to generate random points and use them as positions for spheres
	static std::vector <glm::vec3> generate_random_points(int, float);

	static void integrator(std::vector <aligned_vec3> &,
			       std::vector <aligned_vec3> &,
			       float, float);

	// Pipeline configuration for rendering spheres
	void configure_pipeline(ColorMap);
	void shader_debug();

	void preload(const argparse::ArgumentParser &) override;

	void render(const vk::CommandBuffer &, uint32_t, uint32_t) override;
	void render_imgui(const vk::CommandBuffer &);
	void resize() override;
};

// View and projection information
struct MVP {
	mat4 view;
	mat4 proj;
	f32 smin;
	f32 smax;

	vec4 project_vertex(const vec3 &position, const vec3 &vertex) {
		vec4 p = vec4(vertex + position, 1);
		return proj * (view * p);
	}

	auto layout() {
		return layout_from("MVP",
			verbatim_field(view),
			verbatim_field(proj),
			verbatim_field(smin),
			verbatim_field(smax));
	}
};