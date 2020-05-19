#pragma once

#include "commonVK.h"

namespace MBRF
{

class BufferVK;
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

	void BeginPass(FrameBufferVK* renderTarget);
	void EndPass();

	void ClearRenderTarget(int32_t x, int32_t y, uint32_t width, uint32_t height, VkClearColorValue clearColor, VkClearDepthStencilValue clearDepthStencil);
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);

	void SetVertexBuffer(const BufferVK* vertexBuffer, uint64_t offset);
	void SetIndexBuffer(const BufferVK* indexBuffer, uint64_t offset, bool use16Bits);

	VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
	VkFence m_fence = VK_NULL_HANDLE;
	VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

	FrameBufferVK* m_currentFrameBuffer = nullptr;

	// TODO: add a BufferVK to use as scratch uniforms buffer? Could be a dynamic offset one?
};

}

