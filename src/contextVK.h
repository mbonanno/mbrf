#pragma once

#include "bufferVK.h"
#include "commonVK.h"

#include <unordered_map>

namespace MBRF
{

class DeviceVK;
class FrameBufferVK;
class IndexBufferVK;
class Resource;
class PipelineVK;
class TextureVK;
class VertexBufferVK;

// TODO: add anything related to command buffers recording and submission to this class
// TODO: implement different types: graphics, compute, transfer
class ContextVK
{
public:
	bool Create(DeviceVK* device);
	void Destroy(DeviceVK* device);

	void Begin(DeviceVK* device);
	void End();

	void WaitForLastFrame(DeviceVK* device);
	void Submit(VkQueue queue, VkSemaphore waitSemaphore = VK_NULL_HANDLE, VkSemaphore signalSemaphore = VK_NULL_HANDLE);

	void BeginPass(FrameBufferVK* renderTarget);
	void EndPass();

	void ClearRenderTarget(int32_t x, int32_t y, uint32_t width, uint32_t height, VkClearColorValue clearColor, VkClearDepthStencilValue clearDepthStencil);
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance);
	void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

	void TransitionImageLayout(DeviceVK* device, TextureVK* texture, VkImageLayout newLayout);

	void SetPipeline(PipelineVK* pipeline);

	void SetVertexBuffer(const VertexBufferVK* vertexBuffer, uint64_t offset);
	void SetIndexBuffer(const IndexBufferVK* indexBuffer, uint64_t offset);

	void SetUniformBuffer(BufferVK* buffer, uint32_t bindingSlot);
	void SetUniformBuffer(DeviceVK* device, void* data, uint64_t size, uint32_t bindingSlot);
	void SetTexture(TextureVK* texture, uint32_t bindingSlot);
	void SetStorageImage(TextureVK* texture, uint32_t bindingSlot);

	void CommitBindings(DeviceVK* device);

//private:
	bool CreateDescriptorPools(DeviceVK* device);
	void DestroyDescriptorPools(DeviceVK* device);
	void ResetDescriptorPools(DeviceVK* device);

public:
	VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
	VkFence m_fence = VK_NULL_HANDLE;

	PipelineVK* m_currentPipeline = nullptr;
	FrameBufferVK* m_currentFrameBuffer = nullptr;

	// scratch buffer used for storing frame uniform data
	BufferVK m_uniformScratchBuffer;
	uint32_t m_currentScratchBufferOffset = 0;
	// trackers for the (sub)resources using the scatch buffer memory
	std::vector<BufferRegionVK> m_uniformBufferRegions;

	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
	// layout is going to be fixed, based on shaderCommon.h
	// consider using a freelist of descriptor pools so can allocate as many descriptor sets as we want. Reset every frame

	// add create/update descriptor set functionality

	struct DescriptorBinding
	{
		Resource* m_resource;
		uint32_t m_bindingSlot;
	};

	std::unordered_map<uint32_t, DescriptorBinding> m_uniformBufferBindings;
	std::unordered_map<uint32_t, DescriptorBinding> m_textureBindings;
	std::unordered_map<uint32_t, DescriptorBinding> m_storageImageBindings;

private:
	static const uint32_t s_descriptorPoolMaxSets = 1024;
};

}

