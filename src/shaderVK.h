#pragma once

#include "commonVK.h"

//TODO: add any reflection?

namespace MBRF
{

class DeviceVK;

class ShaderVK
{
public:
	bool CreateFromFile(DeviceVK* device, const char* fileName, VkShaderStageFlagBits stage);
	void Destroy(DeviceVK* device);

	VkShaderModule GetShaderModule() { return m_shaderModule; };
	VkShaderStageFlagBits GetStage() const { return m_stage; };

private:
	VkShaderModule m_shaderModule = VK_NULL_HANDLE;
	VkShaderStageFlagBits m_stage;
};

}
