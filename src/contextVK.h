#pragma once

#include "commonVK.h"

namespace MBRF
{

class DeviceVK;
class FrameBufferVK;

// TODO: add anything related to command buffers recording and submission to this class
// TODO: implement different types: graphics, compute, transfer
class ContextVK
{
public:
	bool Create(DeviceVK* device);
	void Destroy(DeviceVK* device);

	void Begin();
	void End();

	void WaitForLastFrame(DeviceVK* device);
	void Submit(VkQueue queue, VkSemaphore waitSemaphore = VK_NULL_HANDLE, VkSemaphore signalSemaphore = VK_NULL_HANDLE);

	// clearColors[0] = color, clearColors[1] = depth+stencil
	void ClearFramebufferAttachments(const FrameBufferVK* frameBuffer, int32_t x, int32_t y, uint32_t width, uint32_t height, VkClearValue clearValues[2]);

	VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
	VkFence m_fence = VK_NULL_HANDLE;
	VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

	// TODO: add a BufferVK to use as scratch uniforms buffer? Could be a dynamic offset one?
};

}

