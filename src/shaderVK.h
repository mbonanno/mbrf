#pragma once

#include "commonVK.h"

//TODO: add any reflection?

namespace MBRF
{

class DeviceVK;

enum ShaderStage
{
	SHADER_STAGE_VERTEX,
	SHADER_STAGE_FRAGMENT,
	SHADER_STAGE_COMPUTE,
	NUM_SHADER_STAGES,
};

class ShaderVK
{
public:
	bool CreateFromFile(DeviceVK* device, const char* fileName, ShaderStage stage);
	void Destroy(DeviceVK* device);

	VkShaderModule GetShaderModule() { return m_shaderModule; };
	VkShaderStageFlagBits GetStage() const { return m_stage; };

private:
	VkShaderModule m_shaderModule = VK_NULL_HANDLE;
	VkShaderStageFlagBits m_stage;
};

}
