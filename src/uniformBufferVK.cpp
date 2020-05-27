#include "uniformBufferVK.h"

#include "deviceVK.h"

namespace MBRF
{

bool UniformBufferVK::Create(DeviceVK* device, uint64_t size)
{
	m_size = size;

	m_buffers.resize(device->m_swapchainImageCount);

	bool success = true;


	// TODO: for now creating a host visible buffer. Ideally we want to have it in GPU memory, with a transfer context taking care of the updloads each frame?
	for (size_t i = 0; i < m_buffers.size(); ++i)
		success = success && m_buffers[i].Create(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	return success;
}

void UniformBufferVK::Destroy(DeviceVK* device)
{
	for (size_t i = 0; i < m_buffers.size(); ++i)
		m_buffers[i].Destroy(device);

	m_buffers.clear();
}

bool UniformBufferVK::UpdateCurrentBuffer(DeviceVK* device, void* data)
{
	uint32_t swapchainImageIndex = device->m_currentImageIndex;

	assert(swapchainImageIndex < m_buffers.size());

	return m_buffers[swapchainImageIndex].Update(device, m_size, data);
}

BufferVK& UniformBufferVK::GetCurrentBuffer(DeviceVK* device)
{ 
	return m_buffers[device->m_currentImageIndex];
}

}