#include "vertexBufferVK.h"

namespace MBRF
{

// -------------------------------- VertexBufferVK --------------------------------

bool VertexBufferVK::Create(DeviceVK* device, uint64_t size, void* data)
{
	m_size = size;

	m_buffer.Create(device, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	return Update(device, size, data);
}

bool VertexBufferVK::Update(DeviceVK* device, uint64_t size, void* data)
{
	assert(size <= m_size);

	return m_buffer.Update(device, size, data);;
}

void VertexBufferVK::Destroy(DeviceVK* device)
{
	m_buffer.Destroy(device);
}

// -------------------------------- IndexBufferVK --------------------------------

bool IndexBufferVK::Create(DeviceVK* device, uint64_t size, uint32_t numIndices, bool use16Bits, void* data)
{
	m_size = size;
	m_numIndices = numIndices;
	m_use16Bits = use16Bits;

	m_buffer.Create(device, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	return Update(device, size, data);
}

bool IndexBufferVK::Update(DeviceVK* device, uint64_t size, void* data)
{
	assert(size <= m_size);

	return m_buffer.Update(device, size, data);
}

void IndexBufferVK::Destroy(DeviceVK* device)
{
	m_buffer.Destroy(device);
}

}