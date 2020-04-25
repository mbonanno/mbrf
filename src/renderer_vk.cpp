#include "renderer_vk.h"

#include "vulkan/vulkan.h"

#include <algorithm>
#include <iostream>
#include <set>

#define VK_CHECK(func) { VkResult res = func; assert(res == VK_SUCCESS); }

namespace MBRF
{
	// ------------------------------- Validation Layer utils -------------------------------

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

		if (func != nullptr)
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

		if (func != nullptr)
			func(instance, debugMessenger, pAllocator);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	// ------------------------------- RendererVK -------------------------------

	const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	bool RendererVK::CheckExtensionsSupport(const std::vector<const char*>& requiredExtensions, const std::vector<VkExtensionProperties>& availableExtensions)
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

	bool RendererVK::CheckLayersSupport(const std::vector<const char*>& requiredLayers, const std::vector<VkLayerProperties>& availableLayers)
	{
		for (const char* layerName : validationLayers)
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

	bool RendererVK::CreateInstance(bool enableValidation)
	{
		m_validationLayerEnabled = enableValidation;

		uint32_t extensionCount;
		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

		std::vector<VkExtensionProperties> availableExtensions;
		availableExtensions.resize(extensionCount);

		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()));

		// get GLFW required extensions

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		// add validation layer extension
		if (m_validationLayerEnabled)
			requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		// check for extensions support
		if (!CheckExtensionsSupport(requiredExtensions, availableExtensions))
			return false;

		uint32_t layerCount;
		VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

		std::vector<VkLayerProperties> availableLayers(layerCount);
		VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

		// check validation layer support
		CheckLayersSupport(validationLayers, availableLayers);

		VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		appInfo.pNext = nullptr;
		appInfo.pApplicationName = "MBRF";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "MBRF";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

		VkInstanceCreateInfo instInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instInfo.pNext = nullptr;
		instInfo.flags = 0;
		instInfo.pApplicationInfo = &appInfo;
		instInfo.enabledLayerCount = 0;
		instInfo.ppEnabledLayerNames = nullptr;
		instInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		instInfo.ppEnabledExtensionNames = requiredExtensions.data();

		// validation layer
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (m_validationLayerEnabled)
		{
			instInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			instInfo.ppEnabledLayerNames = validationLayers.data();

			debugCreateInfo = {};
			debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugCreateInfo.pfnUserCallback = debugCallback;

			instInfo.pNext = &debugCreateInfo;
		}

		VK_CHECK(vkCreateInstance(&instInfo, nullptr, &m_instance));

		if (m_validationLayerEnabled)
			VK_CHECK(CreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger));

		return true;
	}

	void RendererVK::DestroyInstance()
	{
		if (m_validationLayerEnabled)
			DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);

		vkDestroyInstance(m_instance, nullptr);
	}

	bool RendererVK::CreatePresentationSurface(GLFWwindow* window)
	{
		m_presentationSurface = VK_NULL_HANDLE;
		VK_CHECK(glfwCreateWindowSurface(m_instance, window, nullptr, &m_presentationSurface));

		assert(m_presentationSurface != VK_NULL_HANDLE);

		return m_presentationSurface != VK_NULL_HANDLE;
	}

	void RendererVK::DestroyPresentationSurface()
	{
		vkDestroySurfaceKHR(m_instance, m_presentationSurface, nullptr);
	}

	uint32_t RendererVK::FindDeviceQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags desiredCapabilities)
	{
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies;
		queueFamilies.resize(queueFamilyCount);

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		uint32_t queueFamilyIndex = 0xFFFF;

		for (uint32_t index = 0; index < queueFamilyCount; ++index)
		{
			VkQueueFamilyProperties queueFamily = queueFamilies[index];

			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & desiredCapabilities) != 0)
			{
				queueFamilyIndex = index;

				break;
			}
		}

		return queueFamilyIndex;
	}

	uint32_t RendererVK::FindDevicePresentationQueueFamilyIndex(VkPhysicalDevice device)
	{
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies;
		queueFamilies.resize(queueFamilyCount);

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		uint32_t queueFamilyIndex = 0xFFFF;

		for (uint32_t index = 0; index < queueFamilyCount; ++index)
		{
			VkQueueFamilyProperties queueFamily = queueFamilies[index];

			VkBool32 presentationSupported = VK_FALSE;
			VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_presentationSurface, &presentationSupported);

			if (presentationSupported)
			{
				queueFamilyIndex = index;
				break;
			}
		}

		return queueFamilyIndex;
	}

	bool RendererVK::CreateDevice()
	{
		// Physical Device

		uint32_t devicesCount;
		VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &devicesCount, nullptr));

		if (devicesCount == 0)
		{
			std::cout << "no physical devices available" << std::endl;
			return false;
		}

		std::vector<VkPhysicalDevice> availableDevices;
		availableDevices.resize(devicesCount);

		VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &devicesCount, availableDevices.data()));

		for (uint32_t i = 0; i < devicesCount; ++i)
		{
			VkPhysicalDevice& device = availableDevices[i];

			VkPhysicalDeviceProperties properties;
			VkPhysicalDeviceFeatures features;

			vkGetPhysicalDeviceProperties(device, &properties);
			vkGetPhysicalDeviceFeatures(device, &features);

			// TODO: test for required features

			// check extensions support

			uint32_t extensionCount;
			VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr));

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()));

			bool extensionsFound = true;

			if (!CheckExtensionsSupport(requiredExtensions, availableExtensions))
				continue;

			// check validation layer support

			uint32_t layerCount;
			VK_CHECK(vkEnumerateDeviceLayerProperties(device, &layerCount, nullptr));

			std::vector<VkLayerProperties> availableLayers(layerCount);
			VK_CHECK(vkEnumerateDeviceLayerProperties(device, &layerCount, availableLayers.data()));

			CheckLayersSupport(validationLayers, availableLayers);

			// TODO: check for swapchain

			// check queues support

			m_graphicQueueFamily = FindDeviceQueueFamilyIndex(device, VK_QUEUE_GRAPHICS_BIT);
			m_presentationQueueFamily = FindDevicePresentationQueueFamilyIndex(device);

			if (m_graphicQueueFamily != 0xFFFF && m_presentationQueueFamily != 0xFFFF)
			{
				m_physicalDevice = device;
				m_physicalDeviceProperties = properties;
				m_physicalDeviceFeatures = features;

				break;
			}
		}

		assert(m_physicalDevice != VK_NULL_HANDLE);

		if (!m_physicalDevice)
			return false;

		// Create logical device

		// graphics and presentation queue can have the same family index
		std::set<uint32_t> uniqueQueueFamilies = { m_graphicQueueFamily, m_presentationQueueFamily };

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		float queuePriorities[1] = { 1.0f };

		for (auto queueFamily: uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo graphicsQueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			graphicsQueueCreateInfo.pNext = nullptr;
			graphicsQueueCreateInfo.flags = 0;
			graphicsQueueCreateInfo.queueFamilyIndex = queueFamily;
			graphicsQueueCreateInfo.queueCount = 1;
			graphicsQueueCreateInfo.pQueuePriorities = queuePriorities;
			queueCreateInfos.emplace_back(graphicsQueueCreateInfo);
		}

		VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();
		createInfo.pEnabledFeatures = nullptr;

		VK_CHECK(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device));

		vkGetDeviceQueue(m_device, m_graphicQueueFamily, 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, m_presentationQueueFamily, 0, &m_presentationQueue);

		return true;
	}

	void RendererVK::DestroyDevice()
	{
		vkDestroyDevice(m_device, nullptr);
	}

	bool RendererVK::CreateSwapchain(uint32_t width, uint32_t height)
	{
		// set presentation mode

		VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;  // always supported

		uint32_t presentModesCount;
		VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_presentationSurface, &presentModesCount, nullptr));

		assert(presentModesCount > 0);

		if (presentModesCount == 0)
			return false;

		std::vector<VkPresentModeKHR> supportedPresentModes;
		supportedPresentModes.resize(presentModesCount);
		VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_presentationSurface, &presentModesCount, supportedPresentModes.data()));

		for (auto currentPresentMode : supportedPresentModes)
		{
			if (currentPresentMode == desiredPresentMode)
			{
				presentMode = currentPresentMode;
				break;
			}
		}

		assert(presentMode == desiredPresentMode);

		// set number of images

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_presentationSurface, &surfaceCapabilities));

		uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
		if (surfaceCapabilities.maxImageCount > 0)
			imageCount = std::min(imageCount, surfaceCapabilities.maxImageCount);

		// set swapchain images size

		VkExtent2D imageExtent;

		// check if the window's size is determined by the swapchain images size
		if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
		{
			VkExtent2D minImageSize = surfaceCapabilities.minImageExtent;
			VkExtent2D maxImageSize = surfaceCapabilities.maxImageExtent;

			imageExtent.width = std::min(std::max(width, minImageSize.width), maxImageSize.width);
			imageExtent.height = std::min(std::max(height, minImageSize.height), maxImageSize.height);
		}
		else
		{
			imageExtent = surfaceCapabilities.currentExtent;
		}

		// set images usage

		VkImageUsageFlags desiredUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;  // this usage is always guaranteed. so no real need to check. Adding the check for completeness
		VkImageUsageFlags imageUsage = surfaceCapabilities.supportedUsageFlags & desiredUsage;

		assert(imageUsage == desiredUsage);

		if (imageUsage != desiredUsage)
			return false;

		// set transform (not really required on PC, adding the code for completeness)

		VkSurfaceTransformFlagBitsKHR desiredTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		VkSurfaceTransformFlagBitsKHR surfaceTransform;

		if (desiredTransform & surfaceCapabilities.supportedTransforms)
			surfaceTransform = desiredTransform;
		else
			surfaceTransform = surfaceCapabilities.currentTransform;

		// Set format

		VkSurfaceFormatKHR desiredFormat = { VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		VkFormat imageFormat = VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR imageColorSpace;

		uint32_t formatsCount;
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_presentationSurface, &formatsCount, nullptr));

		assert(formatsCount > 0);

		if (formatsCount == 0)
			return false;

		std::vector<VkSurfaceFormatKHR> supportedFormats;
		supportedFormats.resize(formatsCount);
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_presentationSurface, &formatsCount, supportedFormats.data()));

		// if the only element has VK_FORMAT_UNDEFINED then we are free to choose whatever format and colorspace we want
		if (formatsCount == 1 && supportedFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			imageFormat = desiredFormat.format;
			imageColorSpace = desiredFormat.colorSpace;
		}
		else
		{
			// look for format + colorspace match
			for (auto format : supportedFormats)
			{
				if (format.format == desiredFormat.format && format.colorSpace == desiredFormat.colorSpace)
				{
					imageFormat = desiredFormat.format;
					imageColorSpace = desiredFormat.colorSpace;

					break;
				}
			}

			// if a format + colorspace match wasn't found, look for only the matching format, and accept whatever colorspace is supported with that format
			if (imageFormat == VK_FORMAT_UNDEFINED)
			{
				for (auto format : supportedFormats)
				{
					if (format.format == desiredFormat.format)
					{
						imageFormat = desiredFormat.format;
						imageColorSpace = format.colorSpace;

						break;
					}
				}
			}

			// if a format match wasn't found, accept the first format and colorspace supported available
			if (imageFormat == VK_FORMAT_UNDEFINED)
			{
				imageFormat = supportedFormats[0].format;
				imageColorSpace = supportedFormats[0].colorSpace;
			}	
		}

		assert(imageFormat != VK_FORMAT_UNDEFINED);

		// Create Swapchain

		VkSwapchainCreateInfoKHR swapchainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		swapchainCreateInfo.pNext = nullptr;
		swapchainCreateInfo.flags = 0;
		swapchainCreateInfo.surface = m_presentationSurface;
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = imageFormat;
		swapchainCreateInfo.imageColorSpace = imageColorSpace;
		swapchainCreateInfo.imageExtent = imageExtent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = imageUsage;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;  // number of queue families having access to the image(s) of the swapchain when imageSharingMode is VK_SHARING_MODE_CONCURRENT
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		swapchainCreateInfo.preTransform = surfaceTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		VK_CHECK(vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain));

		// Get handles of swapchain images

		// query imageCount again, as we only required the min number of images. The driver might have created more
		VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr));

		m_swapchainImages.resize(imageCount);
		VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data()));

		return true;
	}

	void RendererVK::DestroySwapchain()
	{
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
	}

	bool RendererVK::CreateSyncObjects(int maxFramesInFlight)
	{
		m_acquireSemaphores.resize(maxFramesInFlight);
		m_renderSemaphores.resize(maxFramesInFlight);
		m_fences.resize(m_swapchainImages.size());

		VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = 0;

		for (int i = 0; i < maxFramesInFlight; ++i)
		{
			VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_acquireSemaphores[i]));
			VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &m_renderSemaphores[i]));
		}

		VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (int i = 0; i < m_fences.size(); ++i)
			VK_CHECK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_fences[i]));

		return true;
	}

	void RendererVK::DestroySyncObjects()
	{
		for (int i = 0; i < m_acquireSemaphores.size(); ++i)
		{
			vkDestroySemaphore(m_device, m_acquireSemaphores[i], nullptr);
			vkDestroySemaphore(m_device, m_renderSemaphores[i], nullptr);
		}

		for (int i = 0; i < m_fences.size(); ++i)
			vkDestroyFence(m_device, m_fences[i], nullptr);
	}

	bool RendererVK::CreateCommandPools()
	{
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.pNext = nullptr;
		createInfo.flags = 0; // optional
		createInfo.queueFamilyIndex = m_graphicQueueFamily;

		VK_CHECK(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_graphicsCommandPool));

		return true;
	}

	void RendererVK::DestroyCommandPools()
	{
		vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
	}

	bool RendererVK::AllocateCommandBuffers()
	{
		m_graphicsCommandBuffers.resize(m_swapchainImages.size());

		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.pNext = nullptr;
		allocateInfo.commandPool = m_graphicsCommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = (uint32_t) m_graphicsCommandBuffers.size();

		VK_CHECK(vkAllocateCommandBuffers(m_device, &allocateInfo, m_graphicsCommandBuffers.data()));

		return true;
	}

	void RendererVK::RecordTestGraphicsCommands()
	{
		// test recording

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional: only relevant for secondary command buffers. It specifies which state to inherit from the calling primary command buffers

		for (int i = 0; i < m_graphicsCommandBuffers.size(); ++i)
		{
			auto commandBuffer = m_graphicsCommandBuffers[i];

			VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

			float clearColor[] = { 1,0,0,1 };

			VkImageSubresourceRange subresourceRange;
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 1;

			VkClearColorValue clearColorValue = { 1.0, 0.0, 0.0, 0.0 };

			// TODO: transition from undefined to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barrier.pNext = nullptr;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_swapchainImages[uint32_t(i)];
			// subresource range
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);



			vkCmdClearColorImage(commandBuffer, m_swapchainImages[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValue, 1, &subresourceRange); //VK_IMAGE_LAYOUT_GENERAL

			// transition the swapchain image to present mode. TODO: need to add command queues to be able to transition

			VkImageMemoryBarrier barrier2 = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barrier2.pNext = nullptr;
			barrier2.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier2.dstAccessMask = 0;
			barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier2.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier2.image = m_swapchainImages[uint32_t(i)];
			// subresource range
			barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier2.subresourceRange.baseMipLevel = 0;
			barrier2.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			barrier2.subresourceRange.baseArrayLayer = 0;
			barrier2.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier2);

			VK_CHECK(vkEndCommandBuffer(commandBuffer));
		}
	}

	bool RendererVK::WaitForDevice()
	{
		VK_CHECK(vkDeviceWaitIdle(m_device));

		return true;
	}

	uint32_t RendererVK::AcquireNextSwapchainImage(int currentFrame)
	{
		uint32_t imageIndex;
		VkResult result =  vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_acquireSemaphores[currentFrame], nullptr, &imageIndex);

		assert(result == VK_SUCCESS);

		switch (result)
		{
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			return imageIndex;
		default:
			return UINT32_MAX;
		}
	}

	void RendererVK::SubmitGraphicsQueue(uint32_t imageIndex, int currentFrame)
	{
		VK_CHECK(vkWaitForFences(m_device, 1, &m_fences[imageIndex], VK_TRUE, UINT64_MAX));
		VK_CHECK(vkResetFences(m_device, 1, &m_fences[imageIndex]));

		VkSemaphore waitSemaphores[] = { m_acquireSemaphores[currentFrame] };
		VkSemaphore signalSemaphores[] = { m_renderSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo;
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_graphicsCommandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_fences[imageIndex]));
	}

	bool RendererVK::PresentSwapchainImage(uint32_t imageIndex, int currentFrame)
	{
		VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_renderSemaphores[currentFrame];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_swapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		VK_CHECK(vkQueuePresentKHR(m_presentationQueue, &presentInfo));

		return true;
	}

}