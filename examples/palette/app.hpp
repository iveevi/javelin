#pragma once

#include <ire.hpp>

#include "common/aperature.hpp"
#include "common/camera_controller.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/vulkan_resources.hpp"
#include "common/imgui.hpp"
#include "common/imported_asset.hpp"
#include "common/transform.hpp"
#include "common/vulkan_triangle_mesh.hpp"
#include "common/application.hpp"

using namespace jvl;
using namespace jvl::ire;

struct Application : CameraApplication {
	littlevk::Pipeline traditional;
	vk::RenderPass render_pass;
	DefaultFramebufferSet framebuffers;

	Transform model_transform;

	float saturation = 0.5f;
	float lightness = 1.0f;
	int splits = 16;

	glm::vec3 min;
	glm::vec3 max;
	bool automatic;

	std::vector <VulkanTriangleMesh> meshes;

	Application();
	~Application();

	void compile_pipeline(VertexFlags, float, float, int);
	void shader_debug();

	void configure(argparse::ArgumentParser &) override;
	void preload(const argparse::ArgumentParser &) override;

	void render(const vk::CommandBuffer &, uint32_t, uint32_t) override;
	void resize() override;
};

// Model-view-projection structure
struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	vec4 project(vec3 position) {
		return proj * (view * (model * vec4(position, 1.0)));
	}

	auto layout() {
		return layout_from("MVP",
			verbatim_field(model),
			verbatim_field(view),
			verbatim_field(proj));
	}
};