#include <littlevk/littlevk.hpp>

#include <engine/imported_asset.hpp>

// Local project headers
#include "editor.hpp"

int main(int argc, char *argv[])
{
	assert(argc >= 2);

	// littlevk configuration
	{
		auto &config = littlevk::config();
		config.enable_logging = false;
		config.abort_on_validation_error = true;
	}

	// TODO: javelin configuration

	Editor editor;

	// Load the asset and scene
	std::filesystem::path path = argv[1];

	auto asset = engine::ImportedAsset::from(path).value();
	editor.scene = core::Scene();
	editor.scene.add(asset);

	// Prepare host and device scenes
	auto host_scene = cpu::Scene::from(editor.scene);
	editor.vk_scene = vulkan::Scene::from(editor.drc.allocator(), host_scene);

	// Main loop
	editor.loop();
}