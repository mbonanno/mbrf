#pragma once

#include "bufferVK.h"

namespace MBRF
{

class UniformBufferVK
{
public:
	bool Create(DeviceVK* device, uint64_t size);
	void Destroy(DeviceVK* device);

	bool UpdateCurrentBuffer(DeviceVK* device, void* data);
	BufferVK& GetCurrentBuffer(DeviceVK* device);

private:
	std::vector<BufferVK> m_buffers;
	uint64_t m_size;
};

}
