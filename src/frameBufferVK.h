#pragma once

#include "commonVK.h"
#include "textureVK.h"

namespace MBRF
{

class DeviceVK;

// TODO: cache render passes
class RenderPassManager
{
public:
	static VkRenderPass GetRenderPass(DeviceVK* device, const std::vector<TextureViewVK> &attachments);
};

class FrameBufferVK
{
public:
	// TODO: create render pass implicitly and cache so we can reuse compatible ones for multiple framebuffers!
	bool Create(DeviceVK* device, uint32_t width, uint32_t height, const std::vector<TextureViewVK> &attachments);
	void Destroy(DeviceVK* device);

	const VkFramebuffer& GetFrameBuffer() const { return m_frameBuffer; };
	const VkRenderPass& GetRenderPass() const { return m_renderPass; };
	const std::vector<TextureViewVK>& GetAttachments() const { return m_attachments; };

private:
	VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;

	uint32_t m_width;
	uint32_t m_height;
	std::vector<TextureViewVK> m_attachments;
};

}
