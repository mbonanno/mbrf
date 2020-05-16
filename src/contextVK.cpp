#include "contextVK.h"

#include "deviceVK.h"

namespace MBRF
{

bool ContextVK::Create(DeviceVK* device)
{
	VkDevice logicDevice = device->GetDevice();

	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.pNext = nullptr;
	allocateInfo.commandPool = device->GetGraphicsCommandPool();
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	VK_CHECK(vkAllocateCommandBuffers(logicDevice, &allocateInfo, &m_commandBuffer));

	VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VK_CHECK(vkCreateFence(logicDevice, &fenceCreateInfo, nullptr, &m_fence));

	return true;
}

void ContextVK::Destroy(DeviceVK* device)
{
	VkDevice logicDevice = device->GetDevice();

	vkDestroyFence(logicDevice, m_fence, nullptr);

	m_commandBuffer = VK_NULL_HANDLE;
	m_fence = VK_NULL_HANDLE;
}

void ContextVK::Begin()
{
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional: only relevant for secondary command buffers. It specifies which state to inherit from the calling primary command buffers

	VK_CHECK(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
}

void ContextVK::End()
{
	VK_CHECK(vkEndCommandBuffer(m_commandBuffer));
}

void ContextVK::Submit(DeviceVK* device, VkQueue queue, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore)
{
	VkDevice logicDevice = device->GetDevice();

	VK_CHECK(vkWaitForFences(logicDevice, 1, &m_fence, VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(logicDevice, 1, &m_fence));

	VkSemaphore waitSemaphores[] = { waitSemaphore };
	VkSemaphore signalSemaphores[] = { signalSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = (waitSemaphore == VK_NULL_HANDLE) ? 0 : 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffer;
	submitInfo.signalSemaphoreCount = (signalSemaphore == VK_NULL_HANDLE) ? 0 : 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, m_fence));
}

void ContextVK::ClearFramebufferAttachments(const FrameBufferVK* frameBuffer, int32_t x, int32_t y, uint32_t width, uint32_t height, VkClearValue clearValues[2])
{
	VkClearRect clearRect;
	clearRect.rect = { {x, y}, {width, height} };
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;

	std::vector<TextureViewVK> attachments = frameBuffer->GetAttachments();

	std::vector<VkClearAttachment> clearAttachments;
	clearAttachments.resize(attachments.size());

	for (size_t i = 0; i < attachments.size(); ++i)
	{
		VkImageAspectFlags aspectMask = attachments[i].GetAspectMask();

		clearAttachments[i].aspectMask = aspectMask;
		clearAttachments[i].colorAttachment = uint32_t(i);

		if (aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
			clearAttachments[i].clearValue.color = clearValues[0].color;
		else if ((aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) || aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)
			clearAttachments[i].clearValue.depthStencil = clearValues[1].depthStencil;
	}

	vkCmdClearAttachments(m_commandBuffer, uint32_t(clearAttachments.size()), clearAttachments.data(), 1, &clearRect);
}

}
