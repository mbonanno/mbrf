#include "bufferVK.h"

#include "deviceVK.h"
#include "utilsVK.h"

namespace MBRF
{

bool BufferVK::Create(DeviceVK* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties)
{
	assert(m_buffer == VK_NULL_HANDLE);

	VkDevice logicDevice = device->GetDevice();
	VkPhysicalDevice physicalDevice = device->GetPhysicalDevice();

	m_size = size;
	m_usage = usage;

	m_hasCpuAccess = memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.size = m_size;
	createInfo.usage = m_usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;

	VK_CHECK(vkCreateBuffer(logicDevice, &createInfo, nullptr, &m_buffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(logicDevice, m_buffer, &memoryRequirements);

	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

	uint32_t memoryType = UtilsVK::FindMemoryType(memoryProperties, memoryRequirements, deviceMemoryProperties);

	assert(memoryType != 0xFFFF);

	if (memoryType == 0xFFFF)
		return false;

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.pNext = nullptr;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryType;

	VK_CHECK(vkAllocateMemory(logicDevice, &allocateInfo, nullptr, &m_memory));

	assert(m_memory != VK_NULL_HANDLE);

	if (!m_memory)
		return false;

	VK_CHECK(vkBindBufferMemory(logicDevice, m_buffer, m_memory, 0));

	if (m_hasCpuAccess)
	{
		// leave it permanently mapped until destruction
		VK_CHECK(vkMapMemory(logicDevice, m_memory, 0, m_size, 0, &m_data));

		assert(m_data);

		if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		{
			VkMappedMemoryRange memoryRanges[1];
			memoryRanges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			memoryRanges[0].pNext = nullptr;
			memoryRanges[0].memory = m_memory;
			memoryRanges[0].offset = 0;
			memoryRanges[0].size = VK_WHOLE_SIZE;

			VK_CHECK(vkFlushMappedMemoryRanges(logicDevice, 1, memoryRanges));
		}
	}

	return true;
}

bool BufferVK::Update(DeviceVK* device, VkDeviceSize size, void* data)
{
	assert(size <= m_size);

	if (m_hasCpuAccess)
	{
		std::memcpy(m_data, data, size);

		return true;
	}

	// if we have no CPU access, we need to create a staging buffer and upload it to GPU

	if (!(m_usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT))
	{
		assert(!"Cannot upload data to device local memory via staging buffer. Usage needs to be VK_BUFFER_USAGE_TRANSFER_DST_BIT.");
		return false;
	}

	BufferVK stagingBuffer;
	stagingBuffer.Create(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

	std::memcpy(stagingBuffer.m_data, data, size);

	// Copy staging buffer to device local memory

	VkCommandBuffer commandBuffer = device->BeginNewCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkBufferCopy region;
	region.srcOffset = 0;
	region.dstOffset = 0;
	region.size = size;

	vkCmdCopyBuffer(commandBuffer, stagingBuffer.m_buffer, m_buffer, 1, &region);

	device->SubmitCommandBufferAndWait(commandBuffer, true);

	stagingBuffer.Destroy(device);

	return true;
}

void BufferVK::Destroy(DeviceVK* device)
{
	VkDevice logicDevice = device->GetDevice();

	vkFreeMemory(logicDevice, m_memory, nullptr);
	vkDestroyBuffer(logicDevice, m_buffer, nullptr);

	m_buffer = VK_NULL_HANDLE;
	m_memory = VK_NULL_HANDLE;
}

}