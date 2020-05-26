#pragma once

#include "bufferVK.h"

namespace MBRF
{

class UniformBufferVK
{
public:
	bool Create(DeviceVK* device, uint32_t swapchainImageCount, uint64_t size);
	bool Update(DeviceVK* device, uint32_t swapchainImageIndex, void* data);
	void Destroy(DeviceVK* device);

	BufferVK& GetBuffer(int index) { return m_buffers[index]; };

private:
	std::vector<BufferVK> m_buffers;
	uint64_t m_size;
};

}
