#include "contextVK.h"

#include "deviceVK.h"
#include "frameBufferVK.h"
#include "pipelineVK.h"
#include "shaderCommon.h"
#include "textureVK.h"
#include "vertexBufferVK.h"

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

	CreateDescriptorPools(device);

	uint32_t size = 1024 * 1024;
	m_uniformScratchBuffer.Create(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_uniformBufferRegions.resize(MAX_UNIFORM_BUFFER_SLOTS);

	return true;
}

void ContextVK::Destroy(DeviceVK* device)
{
	m_uniformScratchBuffer.Destroy(device);

	DestroyDescriptorPools(device);

	vkDestroyFence(device->GetDevice(), m_fence, nullptr);

	m_commandBuffer = VK_NULL_HANDLE;
	m_fence = VK_NULL_HANDLE;
}

bool ContextVK::CreateDescriptorPools(DeviceVK* device)
{
	uint32_t numUniformBuffers = MAX_UNIFORM_BUFFER_SLOTS;
	uint32_t numCombinedImageSamplers = MAX_TEXTURE_SLOTS;
	uint32_t numStorageImages = MAX_STORAGE_IMAGE_SLOTS;

	// Create Descriptor Pool
	VkDescriptorPoolSize poolSizes[3];
	// UBOs
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = numUniformBuffers * s_descriptorPoolMaxSets;
	// Texture + Samplers TODO: create separate samplers?
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = numCombinedImageSamplers * s_descriptorPoolMaxSets;
	// Storage Images
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[2].descriptorCount = numStorageImages * s_descriptorPoolMaxSets;

	VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.maxSets = s_descriptorPoolMaxSets;
	createInfo.poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
	createInfo.pPoolSizes = poolSizes;

	VK_CHECK(vkCreateDescriptorPool(device->GetDevice(), &createInfo, nullptr, &m_descriptorPool));

	return true;
}

void ContextVK::DestroyDescriptorPools(DeviceVK* device)
{
	vkDestroyDescriptorPool(device->GetDevice(), m_descriptorPool, nullptr);

	m_descriptorPool = VK_NULL_HANDLE;
}

void ContextVK::ResetDescriptorPools(DeviceVK* device)
{
	VK_CHECK(vkResetDescriptorPool(device->GetDevice(), m_descriptorPool, 0));

	m_uniformBufferBindings.clear();
	m_textureBindings.clear();
}

void ContextVK::Begin(DeviceVK* device)
{
	m_currentPipeline = nullptr;
	ResetDescriptorPools(device);

	m_currentScratchBufferOffset = 0;

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional: only relevant for secondary command buffers. It specifies which state to inherit from the calling primary command buffers

	VK_CHECK(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
}

void ContextVK::End()
{
	VK_CHECK(vkEndCommandBuffer(m_commandBuffer));
}

void ContextVK::WaitForLastFrame(DeviceVK* device)
{
	VkDevice logicDevice = device->GetDevice();

	VK_CHECK(vkWaitForFences(logicDevice, 1, &m_fence, VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(logicDevice, 1, &m_fence));
}

void ContextVK::Submit(VkQueue queue, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore)
{
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

void ContextVK::BeginPass(FrameBufferVK* renderTarget)
{
	assert(m_currentFrameBuffer == nullptr);

	m_currentFrameBuffer = renderTarget;

	// begin render pass

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_currentFrameBuffer->GetRenderPass();
	renderPassInfo.framebuffer = m_currentFrameBuffer->GetFrameBuffer();

	VkExtent2D rtExtent = { m_currentFrameBuffer->GetWidth(), m_currentFrameBuffer->GetHeight()};

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = rtExtent;

	renderPassInfo.clearValueCount = 0;
	renderPassInfo.pClearValues = nullptr;

	vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void ContextVK::EndPass()
{
	assert(m_currentFrameBuffer != nullptr);

	vkCmdEndRenderPass(m_commandBuffer);
	m_currentFrameBuffer = nullptr;
}

void ContextVK::ClearRenderTarget(int32_t x, int32_t y, uint32_t width, uint32_t height, VkClearColorValue clearColor, VkClearDepthStencilValue clearDepthStencil)
{
	assert(m_currentFrameBuffer != nullptr);

	VkClearValue clearValues[2];
	clearValues[0].color = clearColor;
	clearValues[1].depthStencil = clearDepthStencil;

	VkClearRect clearRect;
	clearRect.rect = { {x, y}, {width, height} };
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;

	std::vector<TextureViewVK> attachments = m_currentFrameBuffer->GetAttachments();

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

void ContextVK::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
{
	vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void ContextVK::SetPipeline(PipelineVK* pipeline)
{
	m_currentPipeline = pipeline;

	vkCmdBindPipeline(m_commandBuffer, m_currentPipeline->GetBindPoint(), pipeline->GetPipeline());
}

void ContextVK::SetVertexBuffer(const VertexBufferVK* vertexBuffer, uint64_t offset)
{
	VkBuffer vbs[] = { vertexBuffer->GetBuffer().GetBuffer() };
	VkDeviceSize offsets[] = { offset };

	vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, vbs, offsets);
}

void ContextVK::SetIndexBuffer(const IndexBufferVK* indexBuffer, uint64_t offset)
{
	vkCmdBindIndexBuffer(m_commandBuffer, indexBuffer->GetBuffer().GetBuffer(), offset, indexBuffer->Use16Bits() ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
}

void ContextVK::SetUniformBuffer(BufferVK* buffer, uint32_t bindingSlot)
{
	assert(bindingSlot < MAX_UNIFORM_BUFFER_SLOTS);

	DescriptorBinding binding = {buffer, bindingSlot};

	m_uniformBufferBindings[bindingSlot] = binding;
}

void ContextVK::SetUniformBuffer(DeviceVK* device, void* data, uint64_t size, uint32_t bindingSlot)
{
	assert(bindingSlot < MAX_UNIFORM_BUFFER_SLOTS);

	DescriptorBinding binding = { &m_uniformBufferRegions[bindingSlot], bindingSlot };

	uint32_t minUniformBufferOffsetAlignment = uint32_t(device->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment);

	m_uniformBufferRegions[bindingSlot].Create(device, &m_uniformScratchBuffer, size, m_currentScratchBufferOffset, data);
	m_currentScratchBufferOffset += uint32_t(size + (minUniformBufferOffsetAlignment - (size % minUniformBufferOffsetAlignment)));

	m_uniformBufferBindings[bindingSlot] = binding;
}

void ContextVK::SetTexture(TextureVK* texture, uint32_t bindingSlot)
{
	assert(bindingSlot < MAX_TEXTURE_SLOTS);

	DescriptorBinding binding = { texture, bindingSlot };

	m_textureBindings[bindingSlot] = binding;
}

void ContextVK::SetStorageImage(TextureVK* texture, uint32_t bindingSlot)
{
	assert(bindingSlot < MAX_STORAGE_IMAGE_SLOTS);

	DescriptorBinding binding = { texture, bindingSlot };

	m_storageImageBindings[bindingSlot] = binding;
}

// TODO: remove the pipelineLayout and add PSO information to ContextVK
void ContextVK::CommitBindings(DeviceVK* device)
{
	if (m_uniformBufferBindings.empty() && m_textureBindings.empty() && m_storageImageBindings.empty())
		return;

	// TODO: cache descriptor sets?

	// Create Descriptor Set

	VkDescriptorSetLayout layout = device->GetDescriptorSetLayout();

	VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.pNext = nullptr;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	allocInfo.descriptorPool = m_descriptorPool;

	VkDescriptorSet descriptorSet;

	VK_CHECK(vkAllocateDescriptorSets(device->GetDevice(), &allocInfo, &descriptorSet));

	// Update Descriptors

	std::vector<VkWriteDescriptorSet> descriptorWrites;

	// UBOs
	for (auto it : m_uniformBufferBindings)
	{
		DescriptorBinding descBinding = it.second;

		VkWriteDescriptorSet wds = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		wds.pNext = nullptr;
		wds.dstSet = descriptorSet;
		wds.dstBinding = UNIFORM_BUFFER_SLOT(descBinding.m_bindingSlot);
		wds.dstArrayElement = 0;
		wds.descriptorCount = 1;
		wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		wds.pImageInfo = nullptr;
		wds.pBufferInfo = &((BufferVK*)descBinding.m_resource)->GetDescriptor();
		wds.pTexelBufferView = nullptr;

		descriptorWrites.emplace_back(wds);
	}

	m_uniformBufferBindings.clear();
	
	// Texture + Samplers
	for (auto it : m_textureBindings)
	{
		DescriptorBinding descBinding = it.second;

		VkWriteDescriptorSet wds = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		wds.pNext = nullptr;
		wds.dstSet = descriptorSet;
		wds.dstBinding = TEXTURE_SLOT(descBinding.m_bindingSlot);
		wds.dstArrayElement = 0;
		wds.descriptorCount = 1;
		wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		wds.pImageInfo = &((TextureVK*)descBinding.m_resource)->GetDescriptor();
		wds.pBufferInfo = nullptr;
		wds.pTexelBufferView = nullptr;

		descriptorWrites.emplace_back(wds);
	}

	m_textureBindings.clear();

	// Storage Images
	for (auto it : m_storageImageBindings)
	{
		DescriptorBinding descBinding = it.second;

		VkWriteDescriptorSet wds = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		wds.pNext = nullptr;
		wds.dstSet = descriptorSet;
		wds.dstBinding = STORAGE_IMAGE_SLOT(descBinding.m_bindingSlot);
		wds.dstArrayElement = 0;
		wds.descriptorCount = 1;
		wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		wds.pImageInfo = &((TextureVK*)descBinding.m_resource)->GetDescriptor();
		wds.pBufferInfo = nullptr;
		wds.pTexelBufferView = nullptr;

		descriptorWrites.emplace_back(wds);
	}

	m_storageImageBindings.clear();
	
	vkUpdateDescriptorSets(device->GetDevice(), uint32_t(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	assert(m_currentPipeline != nullptr);

	vkCmdBindDescriptorSets(m_commandBuffer, m_currentPipeline->GetBindPoint(), m_currentPipeline->GetLayout(), 0, 1, &descriptorSet, 0, nullptr);
}

}
