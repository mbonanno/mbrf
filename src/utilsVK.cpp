#include "utilsVK.h"

#include <assert.h>
#include <iostream>
#include <fstream>

namespace MBRF
{

bool UtilsVK::CheckExtensionsSupport(const std::vector<const char*>& requiredExtensions, const std::vector<VkExtensionProperties>& availableExtensions)
{
	for (const char* extensionName : requiredExtensions)
	{
		bool found = false;

		for (const VkExtensionProperties& extensionProperties : availableExtensions)
		{
			if (strcmp(extensionName, extensionProperties.extensionName) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			std::cout << "extension " << extensionName << " not supported" << std::endl;
			assert(0);
			return false;
		}
	}

	return true;
}

bool UtilsVK::CheckLayersSupport(const std::vector<const char*>& requiredLayers, const std::vector<VkLayerProperties>& availableLayers)
{
	for (const char* layerName : requiredLayers)
	{
		bool found = false;

		for (const VkLayerProperties& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			std::cout << "layer " << layerName << " not supported" << std::endl;
			assert(found);
			return false;
		}
	}

	return true;
}

uint32_t UtilsVK::FindMemoryType(VkMemoryPropertyFlags requiredProperties, VkMemoryRequirements memoryRequirements, VkPhysicalDeviceMemoryProperties deviceMemoryProperties)
{
	for (uint32_t type = 0; type < deviceMemoryProperties.memoryTypeCount; ++type)
	{
		if ((memoryRequirements.memoryTypeBits & (1 << type)) && ((deviceMemoryProperties.memoryTypes[type].propertyFlags & requiredProperties) == requiredProperties))
			return type;
	}

	return 0xFFFF;
}

}