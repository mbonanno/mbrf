#pragma once

#include "commonVK.h"

namespace MBRF
{

class UtilsVK
{
public:
	static bool ReadFile(const char* fileName, std::vector<char> &fileOut);

	static bool CheckExtensionsSupport(const std::vector<const char*>& requiredExtensions, const std::vector<VkExtensionProperties>& availableExtensions);
	static bool CheckLayersSupport(const std::vector<const char*>& requiredLayers, const std::vector<VkLayerProperties>& availableLayers);
};

}


