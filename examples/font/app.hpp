#pragma once

#include <ire.hpp>

#include "common/font.hpp"
#include "common/application.hpp"

using namespace jvl;
using namespace jvl::ire;

struct Application : BaseApplication {
	littlevk::Pipeline raster;
	vk::RenderPass render_pass;
	std::vector <vk::Framebuffer> framebuffers;

	littlevk::Buffer vk_glyphs;
	std::vector <VulkanGlyph> glyphs;
	vk::DescriptorSet descriptor;

	Font font;
	bool automatic;

	Application();

	~Application();

	static void features_include(VulkanFeatureChain &);
	static void features_activate(VulkanFeatureChain &);

	void compile_pipeline(int32_t);
	void shader_debug();

	void preload(const argparse::ArgumentParser &) override;

	void render(const vk::CommandBuffer &, uint32_t, uint32_t) override;
	void imgui(const vk::CommandBuffer &);
	void resize() override;
};