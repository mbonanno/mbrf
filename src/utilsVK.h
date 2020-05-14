#pragma once

#include "commonVK.h"

namespace MBRF
{

class UtilsVK
{
public:
	static bool CheckExtensionsSupport(const std::vector<const char*>& requiredExtensions, const std::vector<VkExtensionProperties>& availableExtensions);
	static bool CheckLayersSupport(const std::vector<const char*>& requiredLayers, const std::vector<VkLayerProperties>& availableLayers);

	static uint32_t FindMemoryType(VkMemoryPropertyFlags requiredProperties, VkMemoryRequirements memoryRequirements, VkPhysicalDeviceMemoryProperties deviceMemoryProperties);
};

}


