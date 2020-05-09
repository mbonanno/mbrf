#include "utilsVK.h"

#include <assert.h>
#include <iostream>
#include <fstream>

namespace MBRF
{

bool UtilsVK::ReadFile(const char* fileName, std::vector<char> &fileOut)
{
	std::ifstream file(fileName, std::ios::in | std::ios::binary | std::ios::ate);

	if (!file.is_open())
	{
		std::cout << "failed to open file " << fileName << std::endl;
		assert(0);
		return false;
	}

	// std::ios::ate: open file at the end, convenient to get the file size
	size_t size = (size_t)file.tellg();
	fileOut.resize(size);

	file.seekg(0, std::ios::beg);
	file.read(fileOut.data(), size);

	file.close();

	return true;
}

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

}