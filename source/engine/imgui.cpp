#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

#include "engine/imgui.hpp"

namespace jvl::engine {

// ImGui utilities
void configure_imgui(DeviceResourceCollection &drc, const vk::RenderPass &render_pass)
{
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(drc.window.handle, true);

	vk::DescriptorPoolSize pool_size {
		vk::DescriptorType::eCombinedImageSampler, 1 << 10
	};

	auto descriptor_pool = littlevk::descriptor_pool(
		drc.device, vk::DescriptorPoolCreateInfo {
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			1 << 10, pool_size,
		}
	).unwrap(drc.dal);

	ImGui_ImplVulkan_InitInfo init_info = {};

	init_info.Instance = littlevk::detail::get_vulkan_instance();
	init_info.DescriptorPool = descriptor_pool;
	init_info.Device = drc.device;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.MinImageCount = 3;
	init_info.PhysicalDevice = drc.phdev;
	init_info.Queue = drc.graphics_queue;
	init_info.RenderPass = render_pass;

	ImGui_ImplVulkan_Init(&init_info);

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	static constexpr const char location[] = "resources/fonts/IosevkaTermNerdFont-Regular.ttf";

	// Load a different font
	io.Fonts->AddFontFromFileTTF(location, 24.0f, nullptr, io.Fonts->GetGlyphRangesDefault());

	ImGui_ImplVulkan_CreateFontsTexture();
	ImGui_ImplVulkan_DestroyFontsTexture();
}

vk::DescriptorSet imgui_texture_descriptor(const vk::Sampler &sampler, const vk::ImageView &view, const vk::ImageLayout &layout)
{
	return ImGui_ImplVulkan_AddTexture(static_cast <VkSampler> (sampler),
			                   static_cast <VkImageView> (view),
					   static_cast <VkImageLayout> (layout));
}

} // namespace jvl::engine