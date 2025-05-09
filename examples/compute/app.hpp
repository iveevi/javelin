#pragma once

#include <ire.hpp>

#include "common/cmaps.hpp"
#include "common/default_framebuffer_set.hpp"
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
	littlevk::Pipeline compute;

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
	vk::DescriptorSet compute_dset;

	std::string cmap = "jet";
	float dt = 0.02f;
	float mass = 500.0f;
	bool pause = false;
	bool automatic;

	Application();

	~Application();

	// Pipeline configuration for rendering spheres
	void configure_render_pipeline(ColorMap);
	void configure_compute_pipeline();
	void shader_debug();

	void preload(const argparse::ArgumentParser &) override;

	void render(const vk::CommandBuffer &, uint32_t, uint32_t) override;
	void render_imgui(const vk::CommandBuffer &);
	void resize() override;

	// Function to generate random points and use them as positions for spheres
	static std::vector <glm::vec3> generate_random_points(int, float);
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
		return layout_from("ViewInfo",
			verbatim_field(view),
			verbatim_field(proj),
			verbatim_field(smin),
			verbatim_field(smax));
	}
};

// Simulation meta data
struct SimulationInfo {
	vec3 O1;
	vec3 O2;
	f32 M;
	f32 dt;
	u32 count;

	auto layout() {
		return layout_from("SimulationInfo",
			verbatim_field(O1),
			verbatim_field(O2),
			verbatim_field(M),
			verbatim_field(dt),
			verbatim_field(count));
	}
};