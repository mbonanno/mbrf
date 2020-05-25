#pragma once

#include "bufferVK.h"

namespace MBRF
{

class VertexBufferVK
{
public:
	bool Create(DeviceVK* device, uint64_t size, void* data);
	bool Update(DeviceVK* device, uint64_t size, void* data);
	void Destroy(DeviceVK* device);

	const BufferVK& GetBuffer() const { return m_buffer; };

private:
	BufferVK m_buffer;
	uint64_t m_size;
};

class IndexBufferVK
{
public:
	bool Create(DeviceVK* device, uint64_t size, uint32_t numIndices, bool use16Bits, void* data);
	bool Update(DeviceVK* device, uint64_t size, void* data);
	void Destroy(DeviceVK* device);

	const BufferVK& GetBuffer() const { return m_buffer; };
	uint32_t GetNumIndices() const { return m_numIndices; };
	bool Use16Bits() const { return m_use16Bits; };

private:
	BufferVK m_buffer;
	uint64_t m_size = 0;
	uint32_t m_numIndices = 0;
	bool m_use16Bits = false;
};

}
