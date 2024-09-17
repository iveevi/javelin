#include <backends/imgui_impl_vulkan.h>

#include "device_resource_collection.hpp"

// Interactive window implementations	
InteractiveWindow::InteractiveWindow(const littlevk::Window &window)
		: littlevk::Window(window) {}

bool InteractiveWindow::key_pressed(int key) const
{
	return glfwGetKey(handle, key) == GLFW_PRESS;
}

// DeviceResourceCollection implementations
littlevk::LinkedDeviceAllocator <> DeviceResourceCollection::allocator()
{
	return littlevk::bind(device, memory_properties, dal);
}

littlevk::LinkedDevices DeviceResourceCollection::combined()
{
	return littlevk::bind(phdev, device);
}

littlevk::LinkedCommandQueue DeviceResourceCollection::commander()
{
	return littlevk::bind(device, command_pool, graphics_queue);
}

void DeviceResourceCollection::configure_device(const std::vector <const char *> &EXTENSIONS)
{
	littlevk::QueueFamilyIndices queue_family = littlevk::find_queue_families(phdev, surface);

	memory_properties = phdev.getMemoryProperties();
	device = littlevk::device(phdev, queue_family, EXTENSIONS);
	dal = littlevk::Deallocator(device);

	graphics_queue = device.getQueue(queue_family.graphics, 0);
	present_queue = device.getQueue(queue_family.present, 0);

	command_pool = littlevk::command_pool(device,
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		queue_family.graphics).unwrap(dal);

	swapchain = combined().swapchain(surface, queue_family);
}

DeviceResourceCollection DeviceResourceCollection::from(const DeviceResourceCollectionInfo &info)
{
	DeviceResourceCollection drc;

	drc.phdev = info.phdev;
	std::tie(drc.surface, drc.window) = littlevk::surface_handles(info.extent, info.title.c_str());
	drc.configure_device(info.extensions);

	return drc;
}

namespace imgui {

// ImGui utilities
void configure_vulkan(DeviceResourceCollection &drc, const vk::RenderPass &render_pass)
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

vk::DescriptorSet add_vk_texture(const vk::Sampler &sampler, const vk::ImageView &view, const vk::ImageLayout &layout)
{
	return ImGui_ImplVulkan_AddTexture(static_cast <VkSampler> (sampler),
			                   static_cast <VkImageView> (view),
					   static_cast <VkImageLayout> (layout));
}

} // namespace imgui