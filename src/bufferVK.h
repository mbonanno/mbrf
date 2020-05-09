#pragma once

#include "commonVK.h"

namespace MBRF
{

class DeviceVK;

class BufferVK
{
public:
	bool Create(DeviceVK* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
	bool Update(DeviceVK* device, VkDeviceSize size, void* data);
	void Destroy(DeviceVK* device);
	

	VkBuffer m_buffer = VK_NULL_HANDLE;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;
	VkDeviceSize m_size = 0;
	VkBufferUsageFlags m_usage;
	void* m_data = nullptr;

	bool m_hasCpuAccess = false;
};

}
