#pragma once

#include "commonVK.h"

namespace MBRF
{

class DeviceVK;

class FrameBufferVK
{
public:
	// TODO: create render pass implicitly and cache so we can reuse compatible ones for multiple framebuffers!
	bool Create(DeviceVK* device, uint32_t width, uint32_t height, const std::vector<VkImageView> &attachments, VkRenderPass renderPass);
	void Destroy(DeviceVK* device);

	const VkFramebuffer& GetFrameBuffer() const { return m_frameBuffer; };

private:
	VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;

	uint32_t m_width;
	uint32_t m_height;
	std::vector<VkImageView> m_attachments;
};

}
