#include <ire.hpp>

#include "common/font.hpp"
#include "common/application.hpp"
#include "common/imgui.hpp"

#include "shaders.hpp"

auto lorem = R"(
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor
incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis
nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.
Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu
fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in
culpa qui officia deserunt mollit anim id est laborum.

Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium
doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore
veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam
voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia
consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque
porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci
velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore
magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum
exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi
consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit
esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo
voluptas nulla pariatur?
)";

struct Application : BaseApplication {
	littlevk::Pipeline raster;
	vk::RenderPass render_pass;
	std::vector <vk::Framebuffer> framebuffers;

	littlevk::Buffer vk_glyphs;
	std::vector <VulkanGlyph> glyphs;
	vk::DescriptorSet descriptor;

	Font font;

	Application() : BaseApplication("Font Rendering", {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
				VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
			}, features_include, features_activate)
	{
		// Configure render passes
		render_pass = littlevk::RenderPassAssembler(resources.device, resources.dal)
			.add_attachment(littlevk::default_color_attachment(resources.swapchain.format))
			.add_subpass(vk::PipelineBindPoint::eGraphics)
				.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
				.done();

		// Configure framebuffers
		littlevk::FramebufferGenerator generator {
			resources.device, render_pass,
			resources.window.extent,
			resources.dal
		};

		for (size_t i = 0; i < resources.swapchain.images.size(); i++)
			generator.add(resources.swapchain.image_views[i]);

		framebuffers = generator.unpack();

		// Configure ImGui
		configure_imgui(resources, render_pass);

		// Load and configure font
		JVL_INFO("loading font...");
		font = Font::from(root() / "resources" / "fonts" / "Arial.ttf");

		JVL_INFO("uploading font glyphs to device...");
		font.upload(resources);

		JVL_INFO("done.");

		// Compile text layout
		auto &extent = resources.window.extent;

		glyphs = font.layout(lorem,
			glm::vec2(extent.width, extent.height),
			glm::vec2(0, 1040.0f),
			30.0f);

		vk_glyphs = resources.allocator()
			.buffer(glyphs, vk::BufferUsageFlagBits::eStorageBuffer);

		// Compile the rendering pipelines
		compile_pipeline(1);

		shader_debug();

		// Bind resources to descsriptor set
		auto alloc_info = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(resources.descriptor_pool)
			.setSetLayouts(raster.dsl.value())
			.setDescriptorSetCount(1);

		descriptor = resources.device.allocateDescriptorSets(alloc_info).front();

		auto vk_glyphs_descriptor = vk_glyphs.descriptor();

		auto write = vk::WriteDescriptorSet()
			.setBufferInfo(vk_glyphs_descriptor)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eStorageBuffer)
			.setDstArrayElement(0)
			.setDstBinding(0)
			.setDstSet(descriptor);

		resources.device.updateDescriptorSets(write, { });
	}

	~Application() {
		shutdown_imgui();
	}

	static void features_include(VulkanFeatureChain &features) {
		features.add <vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR> ();
	}

	static void features_activate(VulkanFeatureChain &features) {
		for (auto &ptr : features) {
			switch (ptr->sType) {
			feature_case(vk::PhysicalDeviceBufferDeviceAddressFeaturesKHR)
				.setBufferDeviceAddress(true);
				break;
			default:
				break;
			}
		}
	}

	void compile_pipeline(int32_t samples) {
		static std::vector <vk::DescriptorSetLayoutBinding> bindings {
			vk::DescriptorSetLayoutBinding()
				.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eStorageBuffer)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex),
		};

		auto fs = procedure("main") << std::make_tuple(samples) << fragment;

		auto vs_spv = link(vertex).generate(Target::spirv_binary_via_glsl, Stage::vertex);
		auto fs_spv = link(fs).generate(Target::spirv_binary_via_glsl, Stage::fragment);

		// TODO: automatic generation by observing used layouts
		auto bundle = littlevk::ShaderStageBundle(resources.device, resources.dal)
			.code(vs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eVertex)
			.code(fs_spv.as <BinaryResult> (), vk::ShaderStageFlagBits::eFragment);

		raster = littlevk::PipelineAssembler <littlevk::PipelineType::eGraphics>
			(resources.device, resources.window, resources.dal)
			.with_render_pass(render_pass, 0)
			.with_shader_bundle(bundle)
			.with_dsl_bindings(bindings)
			.with_push_constant <solid_t <ivec2>> (vk::ShaderStageFlagBits::eVertex)
			.cull_mode(vk::CullModeFlagBits::eNone);
	}

	void render(const vk::CommandBuffer &cmd, uint32_t index, uint32_t) override {
		auto &extent = resources.window.extent;

		// Configure the rendering extent
		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(resources.window.extent));

		littlevk::RenderPassBeginInfo(1)
			.with_render_pass(render_pass)
			.with_framebuffer(framebuffers[index])
			.with_extent(extent)
			.clear_color(0, std::array <float, 4> { 0, 0, 0, 1 })
			.begin(cmd);

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, raster.handle);

		cmd.pushConstants <glm::ivec2> (raster.layout,
			vk::ShaderStageFlagBits::eVertex,
			0, glm::ivec2(extent.width, extent.height));

		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, raster.layout, 0, { descriptor }, { });
		cmd.draw(6, glyphs.size(), 0, 0);

		imgui(cmd);

		cmd.endRenderPass();
	}

	void imgui(const vk::CommandBuffer &cmd) {
		ImGuiRenderContext context(cmd);

		ImGui::Begin("Font Rendering Options");
		{
			static int32_t samples = 1;
			static const std::map <int32_t, std::string> strings {
				{ 1, "No AA"},
				{ 2, "Four Rooks" },
				{ 4, "4x4" },
				{ 8, "8x8" },
			};

			auto &label = strings.at(samples);
			if (ImGui::BeginCombo("Anti-Aliasing", label.c_str())) {
				for (auto &[k, m] : strings) {
					if (ImGui::Selectable(m.c_str(), k == samples)) {
						compile_pipeline(k);
						samples = k;
					}
				}

				ImGui::EndCombo();
			}
		}
		ImGui::End();
	}

	void resize() override {

	}
};

APPLICATION_MAIN()
