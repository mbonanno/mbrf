#include "frameBufferVK.h"

#include "deviceVK.h"

namespace MBRF
{

	bool FrameBufferVK::Create(DeviceVK* device, uint32_t width, uint32_t height, const std::vector<VkImageView> &attachments, VkRenderPass renderPass)
	{
		m_width = width;
		m_height = height;
		m_attachments.assign(attachments.begin(), attachments.end());

		VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = uint32_t(attachments.size());
		createInfo.pAttachments = attachments.data();
		createInfo.width = width;
		createInfo.height = height;
		createInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(device->GetDevice(), &createInfo, nullptr, &m_frameBuffer));

		return true;
	}

	void FrameBufferVK::Destroy(DeviceVK* device)
	{
		vkDestroyFramebuffer(device->GetDevice(), m_frameBuffer, nullptr);

		m_frameBuffer = VK_NULL_HANDLE;
		m_attachments.clear();
	}

}