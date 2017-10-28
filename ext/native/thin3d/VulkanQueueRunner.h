#pragma once

#include <cstdint>

#include "Common/Vulkan/VulkanContext.h"
#include "math/dataconv.h"
#include "thin3d/thin3d.h"

class VKRFramebuffer;
struct VKRImage;

enum class VKRRenderCommand : uint8_t {
	BIND_PIPELINE,
	STENCIL,
	BLEND,
	VIEWPORT,
	SCISSOR,
	CLEAR,
	DRAW,
	DRAW_INDEXED,
};

struct VkRenderData {
	VKRRenderCommand cmd;
	union {
		struct {
			VkPipeline pipeline;
		} pipeline;
		struct {
			VkPipelineLayout pipelineLayout;
			VkDescriptorSet ds;
			int numUboOffsets;
			uint32_t uboOffsets[3];
			VkBuffer vbuffer;
			VkDeviceSize voffset;
			uint32_t count;
		} draw;
		struct {
			VkPipelineLayout pipelineLayout;
			VkDescriptorSet ds;
			int numUboOffsets;
			uint32_t uboOffsets[3];
			VkBuffer vbuffer;  // might need to increase at some point
			VkDeviceSize voffset;
			VkBuffer ibuffer;
			VkDeviceSize ioffset;
			uint32_t count;
			int16_t instances;
			VkIndexType indexType;
		} drawIndexed;
		struct {
			uint32_t clearColor;
			float clearZ;
			int clearStencil;
			int clearMask;   // VK_IMAGE_ASPECT_COLOR_BIT etc
		} clear;
		struct {
			VkViewport vp;
		} viewport;
		struct {
			VkRect2D scissor;
		} scissor;
		struct {
			uint8_t stencilWriteMask;
			uint8_t stencilCompareMask;
			uint8_t stencilRef;
		} stencil;
		struct {
			float color[4];
		} blendColor;
		struct {

		} beginRp;
		struct {

		} endRp;
	};
};

enum class VKRStepType : uint8_t {
	RENDER,
	COPY,
	BLIT,
	READBACK,
};

enum class VKRRenderPassAction {
	DONT_CARE,
	CLEAR,
	KEEP,
};

struct TransitionRequest {
	VKRFramebuffer *fb;
	VkImageLayout targetLayout;
};

struct VKRStep {
	VKRStep(VKRStepType _type) : stepType(_type) {}
	VKRStepType stepType;
	std::vector<VkRenderData> commands;
	std::vector<TransitionRequest> preTransitions;
	union {
		struct {
			VKRFramebuffer *framebuffer;
			VKRRenderPassAction color;
			VKRRenderPassAction depthStencil;
			uint32_t clearColor;
			float clearDepth;
			int clearStencil;
			int numDraws;
			VkImageLayout finalColorLayout;
		} render;
		struct {
			VKRFramebuffer *src;
			VKRFramebuffer *dst;
			VkRect2D srcRect;
			VkOffset2D dstPos;
			int aspectMask;
		} copy;
		struct {
			VKRFramebuffer *src;
			VKRFramebuffer *dst;
			VkRect2D srcRect;
			VkRect2D dstRect;
			int aspectMask;
			VkFilter filter;
		} blit;
		struct {
			VKRFramebuffer *src;
			void *destPtr;
			VkRect2D srcRect;
		} readback;
	};
};

class VulkanQueueRunner {
public:
	VulkanQueueRunner(VulkanContext *vulkan) : vulkan_(vulkan) {}
	void SetBackbuffer(VkFramebuffer fb) {
		backbuffer_ = fb;
	}
	void RunSteps(VkCommandBuffer cmd, const std::vector<VKRStep *> &steps);

	void CreateDeviceObjects();
	void DestroyDeviceObjects();

	VkRenderPass GetBackbufferRenderPass() const {
		return backbufferRenderPass_;
	}
	VkRenderPass GetRenderPass(int i) const {
		return renderPasses_[i];
	}

	inline int RPIndex(VKRRenderPassAction color, VKRRenderPassAction depth) {
		return (int)depth * 3 + (int)color;
	}

private:
	void InitBackbufferRenderPass();
	void InitRenderpasses();

	void PerformBindFramebufferAsRenderTarget(const VKRStep &pass, VkCommandBuffer cmd);
	void PerformRenderPass(const VKRStep &pass, VkCommandBuffer cmd);
	void PerformCopy(const VKRStep &pass, VkCommandBuffer cmd);
	void PerformBlit(const VKRStep &pass, VkCommandBuffer cmd);

	static void SetupTransitionToTransferSrc(VKRImage &img, VkImageMemoryBarrier &barrier, VkPipelineStageFlags &stage, VkImageAspectFlags aspect);
	static void SetupTransitionToTransferDst(VKRImage &img, VkImageMemoryBarrier &barrier, VkPipelineStageFlags &stage, VkImageAspectFlags aspect);

	VulkanContext *vulkan_;

	VkFramebuffer backbuffer_;
	VkFramebuffer curFramebuffer_ = VK_NULL_HANDLE;

	VkRenderPass backbufferRenderPass_ = VK_NULL_HANDLE;
	// Renderpasses, all combinations of preserving or clearing or dont-care-ing fb contents.
	// TODO: Create these on demand.
	VkRenderPass renderPasses_[9]{};
};