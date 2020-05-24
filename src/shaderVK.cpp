#include "shaderVK.h"

#include "deviceVK.h"
#include "utils.h"

namespace MBRF
{

bool ShaderVK::CreateFromFile(DeviceVK* device, const char* fileName)
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

	return true;
}

void ShaderVK::Destroy(DeviceVK* device)
{
	vkDestroyShaderModule(device->GetDevice(), m_shaderModule, nullptr);

	m_shaderModule = VK_NULL_HANDLE;
}

}