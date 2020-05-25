#pragma once

#include "commonVK.h"
#include "resource.h"

namespace MBRF
{

class DeviceVK;

class BufferVK : public Resource
{
public:
	bool Create(DeviceVK* device, uint64_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
	bool Update(DeviceVK* device, uint64_t size, void* data);
	void Destroy(DeviceVK* device);

	const VkBuffer GetBuffer() const { return m_buffer; };
	void* GetData() { return m_data; };

	const VkDescriptorBufferInfo& GetDescriptor() const { return m_descriptor; };
	
private:
	VkBuffer m_buffer = VK_NULL_HANDLE;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;
	uint64_t m_size = 0;

	VkDescriptorBufferInfo m_descriptor;

	VkBufferUsageFlags m_usage;
	void* m_data = nullptr;

	bool m_hasCpuAccess = false;
	bool m_hasCoherentMemory = false;
};

}
