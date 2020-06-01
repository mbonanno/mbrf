#include "shaderVK.h"

#include "deviceVK.h"
#include "utils.h"

#include <iostream>

namespace MBRF
{

bool ShaderVK::CreateFromFile(DeviceVK* device, const char* fileName, ShaderStage stage)
{
	std::vector<char> shaderCode;

	if (!Utils::ReadFile(fileName, shaderCode))
		return false;

	VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode.data());

	VK_CHECK(vkCreateShaderModule(device->GetDevice(), &createInfo, nullptr, &m_shaderModule));

	switch (stage)
	{
	case SHADER_STAGE_VERTEX:
		m_stage = VK_SHADER_STAGE_VERTEX_BIT;
		break;
	case SHADER_STAGE_FRAGMENT:
		m_stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		break;
	case SHADER_STAGE_COMPUTE:
		m_stage = VK_SHADER_STAGE_COMPUTE_BIT;
		break;
	default:
		std::cout << "Shader stage not implemented yet" << std::endl;
		assert(0);
		return false;
	}

	return true;
}

void ShaderVK::Destroy(DeviceVK* device)
{
	vkDestroyShaderModule(device->GetDevice(), m_shaderModule, nullptr);

	m_shaderModule = VK_NULL_HANDLE;
}

}