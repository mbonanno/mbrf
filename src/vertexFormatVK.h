#pragma once

#include "commonVK.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // use the Vulkan range of 0.0 to 1.0, instead of the -1 to 1.0 OpenGL range
#include "glm.hpp"

namespace MBRF
{

struct TestVertex
{
	glm::vec3 m_pos;
	glm::vec4 m_color;
	glm::vec2 m_texcoord;

	static VkVertexInputBindingDescription GetBindingDescription() 
	{
		VkVertexInputBindingDescription bindingDescription;
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(TestVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		attributeDescriptions.resize(3);

		// pos
		attributeDescriptions[0].location = 0; // shader input location
		attributeDescriptions[0].binding = 0; // vertex buffer binding
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(TestVertex, m_pos);
		// color
		attributeDescriptions[1].location = 1; // shader input location
		attributeDescriptions[1].binding = 0; // vertex buffer binding
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(TestVertex, m_color);
		// texcoord
		attributeDescriptions[2].location = 2; // shader input location
		attributeDescriptions[2].binding = 0; // vertex buffer binding
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(TestVertex, m_texcoord);

		return attributeDescriptions;
	}
	

	
};

}
