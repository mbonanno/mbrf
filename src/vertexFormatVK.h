#pragma once

#include "commonVK.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // use the Vulkan range of 0.0 to 1.0, instead of the -1 to 1.0 OpenGL range
#include "glm.hpp"

namespace MBRF
{

struct VertexAttributeVK
{
	VertexAttributeVK(uint32_t location, VkFormat format, uint32_t offset, uint32_t size): m_location(location), m_format(format), m_offset(offset), m_size(size) {};

	uint32_t m_location;
	VkFormat m_format;
	uint32_t m_offset;
	uint32_t m_size;
};

class VertexFormatVK
{
public:
	VertexFormatVK(uint32_t binding=0): m_binding(binding) {};

	void AddAttribute(const VertexAttributeVK &attribute);

	VkVertexInputBindingDescription GetBindingDescription() const;
	std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() const;

	void SetBinding(uint32_t binding) { m_binding = binding; };

private:
	uint32_t m_binding = 0;
	uint32_t m_stride = 0;
	std::vector<VertexAttributeVK> m_attributes;
};

}
