#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BBOX_H
#include FT_OUTLINE_H

#include <ire.hpp>

#include "common/extensions.hpp"
#include "common/default_framebuffer_set.hpp"
#include "common/vulkan_resources.hpp"
#include "common/imgui.hpp"

using namespace jvl;
using namespace jvl::ire;

MODULE(font-rendering);

VULKAN_EXTENSIONS(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

#define NUMBER_OF_GLYPHS 96

// Adapted from https://github.com/bwoods/Vulkan
struct Rectangle {
	float min_x;
	float min_y;
	float max_x;
	float max_y;
	// TODO: float4
};

struct ContourRange {
	uint32_t begin;
	uint32_t end;
	// TODO: int2
};

struct Outline {
	Rectangle bbox;
	std::vector <glm::vec2> points;
	std::vector <ContourRange> contours;

	void add_point(const glm::vec2 &point) {
		points.emplace_back(point);
	}

	void add_contour(const ContourRange &range) {
		contours.emplace_back(range);
	}
};

// static void outline_add_odd_point(Outline *o)
// {
// 	if (o->points.size() % 2 != 0) {
// 		float2 p = { o->bbox.min_x, o->bbox.min_y };
// 		o->add_point(p);
// 	}
// }

static inline glm::vec2 convert_point(const FT_Vector *v)
{
	return glm::vec2(v->x / 64.0f, v->y / 64.0f);
}

static int move_to_func(const FT_Vector *to, Outline *o)
{
	JVL_INFO("\t{} c={},p={}", __FUNCTION__,
		o->contours.size(),
		o->points.size());

	glm::vec2 p { 0, 0 };

	size_t ncontours = o->contours.size();
	size_t npoints = o->points.size();

	fmt::println("\tafter c={},p={}",
		o->contours.size(),
		o->points.size());

	if (ncontours) {
		o->contours[ncontours - 1].end = npoints - 1;
		o->add_point(p);
	}

	fmt::println("\tafter c={},p={}",
		o->contours.size(),
		o->points.size());

	JVL_ASSERT_PLAIN(o->points.size() % 2 == 0);

	ContourRange range {
		uint32_t(o->points.size()),
		UINT32_MAX
	};

	o->add_contour(range);

	p = convert_point(to);
	o->add_point(p);

	return 0;
}

static int line_to_func(const FT_Vector *to, Outline *o)
{
	JVL_INFO("\t{} c={},p={}", __FUNCTION__,
		o->contours.size(),
		o->points.size());

	uint32_t last = o->points.size() - 1;

	glm::vec2 p, to_p;

	to_p = convert_point(to);
	p = 0.5f * (o->points[last] + to_p);

	o->add_point(p);
	o->add_point(to_p);

	return 0;
}

static int conic_to_func(const FT_Vector *control,
			 const FT_Vector* to,
			 Outline *o)
{
	JVL_INFO("\t{}", __FUNCTION__);

	glm::vec2 p;

	p = convert_point(control);
	o->add_point(p);

	p = convert_point(to);
	o->add_point(p);

	return 0;
}

static int cubic_to_func(const FT_Vector *control1,
			 const FT_Vector *control2,
			 const FT_Vector *to,
			 Outline *o)
{
	JVL_INFO("\t{}", __FUNCTION__);

	return line_to_func(to, o);
}

void decompose_outline(FT_Outline *outline, Outline &o)
{
	FT_BBox outline_bbox;
	JVL_ASSERT_PLAIN(!FT_Outline_Get_BBox(outline, &outline_bbox));

	o.bbox.min_x = (float) outline_bbox.xMin / 64.0f;
	o.bbox.min_y = (float) outline_bbox.yMin / 64.0f;
	o.bbox.max_x = (float) outline_bbox.xMax / 64.0f;
	o.bbox.max_y = (float) outline_bbox.yMax / 64.0f;

	FT_Outline_Funcs funcs {
		.move_to = reinterpret_cast <FT_Outline_MoveToFunc> (move_to_func),
		.line_to = reinterpret_cast <FT_Outline_LineToFunc> (line_to_func),
		.conic_to = reinterpret_cast <FT_Outline_ConicToFunc> (conic_to_func),
		.cubic_to = reinterpret_cast <FT_Outline_CubicToFunc> (cubic_to_func),
	};

	JVL_ASSERT_PLAIN(!FT_Outline_Decompose(outline, &funcs, &o));

	size_t ncontours = o.contours.size();
	size_t npoints = o.points.size();
	if (ncontours)
		o.contours[ncontours - 1].end = npoints - 1;
}

int main()
{
	// Process the font
	const char *const font = "resources/fonts/IosevkaTermNerdFont-Regular.ttf";

	JVL_ASSERT_PLAIN(std::filesystem::exists(font));

	FT_Library library;
	JVL_ASSERT_PLAIN(!FT_Init_FreeType(&library));
	fmt::println("library: {}", (void *) library);

	FT_Face face;
	JVL_ASSERT_PLAIN(!FT_New_Face(library, font, 0, &face));
	fmt::println("face: {}", (void *) face);

	for (uint32_t i = 0; i < NUMBER_OF_GLYPHS; i++) {
		char c = ' ' + i;

		FT_UInt glyph_index = FT_Get_Char_Index(face, c);
		JVL_ASSERT_PLAIN(!FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING));
		fmt::println("processing character: {} -> {}", c, glyph_index);

		Outline outline;
		decompose_outline(&face->glyph->outline, outline);
	}

	// littlevk configuration
	{
		auto &config = littlevk::config();
		config.enable_logging = false;
		config.abort_on_validation_error = true;
	}

	// Configure the resource collection
	auto predicate = [&](vk::PhysicalDevice phdev) {
		return littlevk::physical_device_able(phdev, VK_EXTENSIONS);
	};

	auto phdev = littlevk::pick_physical_device(predicate);

	auto drc = VulkanResources::from(phdev, "Font", vk::Extent2D(1920, 1080), VK_EXTENSIONS);

	// Create the render pass and generate the pipelines
	vk::RenderPass render_pass = littlevk::RenderPassAssembler(drc.device, drc.dal)
		.add_attachment(littlevk::default_color_attachment(drc.swapchain.format))
		.add_attachment(littlevk::default_depth_attachment())
		.add_subpass(vk::PipelineBindPoint::eGraphics)
			.color_attachment(0, vk::ImageLayout::eColorAttachmentOptimal)
			.depth_attachment(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.done();

	// Configure ImGui
	configure_imgui(drc, render_pass);

	// Framebuffer manager
	DefaultFramebufferSet framebuffers;
	framebuffers.resize(drc, render_pass);

	// Command buffers for the rendering loop
	auto command_buffers = littlevk::command_buffers(drc.device,
		drc.command_pool,
		vk::CommandBufferLevel::ePrimary, 2u);

	// Synchronization information
	auto frame = 0u;
	auto sync = littlevk::present_syncronization(drc.device, 2).unwrap(drc.dal);

	// Handling window resizing
	auto resizer = [&]() { framebuffers.resize(drc, render_pass); };

	// Main loop
	auto &window = drc.window;
	while (!window.should_close()) {
		window.poll();
		
		const auto &cmd = command_buffers[frame];
		const auto &sync_frame = sync[frame];
		
		auto sop = littlevk::acquire_image(drc.device, drc.swapchain.handle, sync_frame);
		if (sop.status == littlevk::SurfaceOperation::eResize) {
			resizer();
			continue;
		}

		// Start the command buffer
		cmd.begin(vk::CommandBufferBeginInfo());

		// Configure the rendering extent
		littlevk::viewport_and_scissor(cmd, littlevk::RenderArea(drc.window.extent));

		littlevk::RenderPassBeginInfo(2)
			.with_render_pass(render_pass)
			.with_framebuffer(framebuffers[sop.index])
			.with_extent(drc.window.extent)
			.clear_color(0, std::array <float, 4> { 0, 0, 0, 1 })
			.clear_depth(1, 1)
			.begin(cmd);

		cmd.endRenderPass();
		cmd.end();

		// Submit command buffer while signaling the semaphore
		vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		
		vk::SubmitInfo submit_info {
			sync_frame.image_available,
			wait_stage, cmd,
			sync_frame.render_finished
		};

		drc.graphics_queue.submit(submit_info, sync_frame.in_flight);

		sop = littlevk::present_image(drc.present_queue, drc.swapchain.handle, sync_frame, sop.index);
		if (sop.status == littlevk::SurfaceOperation::eResize)
			resizer();

		// Advance to the next frame
		frame = 1 - frame;
	}
}
