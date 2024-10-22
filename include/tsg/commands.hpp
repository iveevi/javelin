#pragma once

#include <vulkan/vulkan.hpp>

#include "artifact.hpp"
#include "pipeline.hpp"
#include "filters.hpp"

namespace jvl::tsg {

enum CommandState {
	eReady,
	eRenderPass,
	eGraphicsPipelineBound,
};

class CommandBufferBase {
protected:
	const vk::CommandBuffer &cmd;
	CommandBufferBase(const vk::CommandBuffer &cmd_) : cmd(cmd_) {}
};

template <CommandState C = eReady, specifier ... Args>
struct CommandBuffer : CommandBufferBase {
	CommandBuffer(const vk::CommandBuffer &cmd) : CommandBufferBase(cmd) {
		vk::CommandBufferBeginInfo info;
		cmd.begin(info);
	}

	~CommandBuffer() {
		cmd.end();
	}

	// TODO: alternative which progressively adds options...
	// TODO: and sets default clear colors based on a templated render pass

	auto beginRenderPass(const vk::RenderPassBeginInfo &info) {
		return CommandBuffer <eRenderPass> (*this, info);
	}
};

template <specifier ... Args>
struct CommandBuffer <eRenderPass, Args...> : CommandBufferBase {
	CommandBuffer(const CommandBufferBase &base, const vk::RenderPassBeginInfo &info)
			: CommandBufferBase(base) {
		cmd.beginRenderPass(info, vk::SubpassContents::eInline);
	}

	~CommandBuffer() {
		cmd.endRenderPass();
	}

	// TODO: each of these should be a separate state?
	auto &setViewport(const vk::Viewport &viewport) {
		cmd.setViewport(0, viewport);
		return *this;
	}

	auto &setScissor(const vk::Rect2D &scissor) {
		cmd.setScissor(0, scissor);
		return *this;
	}

	template <ShaderStageFlags Flags, specifier ... Specifiers>
	auto bindPipeline(const Pipeline <Flags, Specifiers...> &pipeline) {
		if constexpr (Flags == ShaderStageFlags::eGraphicsVertexFragment) {
			cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle);
			return CommandBuffer <eGraphicsPipelineBound, Specifiers...> (*this, pipeline.layout);
		}
	}
};

// TODO: template buffers for Vulkan using solid structures...
// TODO: additional flags to check which push constants have been set...
template <specifier ... Args>
struct CommandBuffer <eGraphicsPipelineBound, Args...> : CommandBufferBase {
	using VertexPushConstants = filter_stage_push_constants_impl <ShaderStageFlags::eVertex, Args...> ::result;
	using FragmentPushConstants = filter_stage_push_constants_impl <ShaderStageFlags::eFragment, Args...> ::result;

	vk::PipelineLayout layout;

	CommandBuffer(const CommandBufferBase &other, const vk::PipelineLayout &layout_)
			: CommandBufferBase(other), layout(layout_) {}

	// ~CommandBuffer() = delete;

	// TODO: fetch vertex push constants...
	auto &pushConstantsVertex(const solid_t <typename VertexPushConstants::type> &value)
	requires (!std::same_as <VertexPushConstants, NullPushConstants>) {
		using T = VertexPushConstants::type;

		cmd.pushConstants <solid_t <T>> (layout,
			vk::ShaderStageFlagBits::eVertex,
			VertexPushConstants::offset, value);

		return *this;
        }
	
	auto &pushConstantsFragment(const solid_t <typename FragmentPushConstants::type> &value)
	requires (!std::same_as <FragmentPushConstants, NullPushConstants>) {
		using T = FragmentPushConstants::type;

		cmd.pushConstants <solid_t <T>> (layout,
			vk::ShaderStageFlagBits::eFragment,
			FragmentPushConstants::offset, value);
		
		return *this;
        }

	// TODO: requires vertex push constants...

	// TODO: intermediate states for this?
	auto &draw(const vk::Buffer &index, const vk::Buffer &vertex, size_t count) {
		cmd.bindVertexBuffers(0, vertex, { 0 });
		cmd.bindIndexBuffer(index, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(count, 1, 0, 0, 0);
		
		return *this;
	}
};

} // namespace jvl::tsg