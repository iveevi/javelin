#include <cassert>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <memory>

#include <fmt/format.h>
#include <fmt/printf.h>
#include <fmt/std.h>

// Local project headers
#include "glio.hpp"
#include "device_resource_collection.hpp"
#include "global_context.hpp"
#include "attachment_raytracing_cpu.hpp"
#include "attachment_viewport.hpp"
#include "attachment_inspectors.hpp"
#include "attachment_ui.hpp"

using namespace jvl;

using namespace jvl::core;
using namespace jvl::gfx;

int main(int argc, char *argv[])
{
	littlevk::config().enable_logging = false;
	littlevk::config().abort_on_validation_error = true;

	assert(argc >= 2);

	// Device extensions
	static const std::vector <const char *> EXTENSIONS {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	// Load physical device
	auto predicate = [](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, EXTENSIONS);
	};

	GlobalContextPrelude prelude;
	prelude.phdev = littlevk::pick_physical_device(predicate);
	prelude.extent = vk::Extent2D(1920, 1080);
	prelude.title = "Javelin Editor";
	prelude.extensions = EXTENSIONS;

	auto global_context = std::make_unique <GlobalContext> (prelude);
	auto &drc = global_context->drc;

	std::filesystem::path path = argv[1];
	fmt::println("path to asset: {}", path);

	auto asset = engine::ImportedAsset::from(path).value();

	global_context->scene.add(asset);
	global_context->scene.write("main.jvlx");

	// Engine editor attachments
	auto attatchment_ui = std::make_unique <AttachmentUI> (global_context);
	auto attachment_viewport = std::make_unique <AttachmentViewport> (global_context);
	auto attachment_inspectors = std::make_unique <AttachmentInspectors> (*global_context);
	auto attachment_rtx_cpu = std::make_unique <AttachmentRaytracingCPU> (*global_context);

	for (auto &obj : global_context->scene.objects) {
		if (obj.geometry) {
			auto &g = obj.geometry.value();

			auto tmesh = core::TriangleMesh::from(g).value();
			auto vmesh = gfx::vulkan::TriangleMesh::from(drc.allocator(), tmesh).value();
			attachment_viewport->meshes.push_back(vmesh);
		}
	}

	glfwSetWindowUserPointer(drc.window.handle, &attachment_viewport->transform);
	glfwSetMouseButtonCallback(drc.window.handle, button_callback);
	glfwSetCursorPosCallback(drc.window.handle, cursor_callback);

	attatchment_ui->subscribe(&AttachmentInspectors::scene_hierarchy, attachment_inspectors.get());
	attatchment_ui->subscribe(&AttachmentInspectors::object_inspector, attachment_inspectors.get());
	attatchment_ui->subscribe(&AttachmentViewport::render_imgui, attachment_viewport.get());
	attatchment_ui->subscribe(&AttachmentRaytracingCPU::render_imgui_synthesized, attachment_rtx_cpu.get());
	attatchment_ui->subscribe(&AttachmentRaytracingCPU::render_imgui_options, attachment_rtx_cpu.get());

	global_context->attach(AttachmentRaytracingCPU::key, attachment_rtx_cpu->attachment());
	global_context->attach("viewport", attachment_viewport->attachment());
	global_context->attach("ui", attatchment_ui->attachment());

	global_context->loop();

	drc.device.waitIdle();
	drc.window.drop();
	drc.dal.drop();

	attachment_rtx_cpu->thread_pool->drop();
}
