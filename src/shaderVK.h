#pragma once

#include "commonVK.h"

//TODO: add any reflection?

namespace MBRF
{

class DeviceVK;

class ShaderVK
{
public:
	bool CreateFromFile(DeviceVK* device, const char* fileName);
	void Destroy(DeviceVK* device);

	VkShaderModule GetShaderModule() { return m_shaderModule; };

private:
	VkShaderModule m_shaderModule = VK_NULL_HANDLE;
};

}
