#include "vertexFormatVK.h"

namespace MBRF
{

void VertexFormatVK::AddAttribute(const VertexAttributeVK &attribute)
{
	m_attributes.emplace_back(attribute);
	m_stride += attribute.m_size;
}

VkVertexInputBindingDescription VertexFormatVK::GetBindingDescription() const
{
	VkVertexInputBindingDescription bindingDescription;
	bindingDescription.binding = 0;
	bindingDescription.stride = m_stride;
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> VertexFormatVK::GetAttributeDescriptions() const
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	
	for (const auto &attribute : m_attributes)
	{
		VkVertexInputAttributeDescription desc;

		desc.location = attribute.m_location; // shader input location
		desc.binding = m_binding; // vertex buffer binding
		desc.format = attribute.m_format;
		desc.offset = attribute.m_offset;

		attributeDescriptions.emplace_back(desc);
	}

	return attributeDescriptions;
}

}