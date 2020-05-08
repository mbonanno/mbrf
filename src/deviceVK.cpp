#include "deviceVK.h"

#include <iostream>
#include <set>

namespace MBRF
{

//// ------------------------------- Validation Layer utils -------------------------------

//VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
//{
//	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

//	if (func != nullptr)
//		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
//	else
//		return VK_ERROR_EXTENSION_NOT_PRESENT;
//}

//void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
//{
//	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

//	if (func != nullptr)
//		func(instance, debugMessenger, pAllocator);
//}

//VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
//	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//	VkDebugUtilsMessageTypeFlagsEXT messageType,
//	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
//	void* pUserData)
//{
//	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

//	return VK_FALSE;
//}

//// ------------------------------- DeviceVK -------------------------------

//const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
//const std::vector<const char*> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };




	



}