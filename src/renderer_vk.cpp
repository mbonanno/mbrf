#include "renderer_vk.h"

#include <iostream>
#include <fstream>
#include <set>

namespace MBRF
{
	// ------------------------------- Utils -------------------------------

	bool Utils::ReadFile(const char* fileName, std::vector<char> &fileOut)
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

	bool RendererVK::Init(GLFWwindow* window, uint32_t width, uint32_t height)
	{
		m_currentFrame = 0;

		CreateInstance(true);

		m_swapchain.SetInstance(m_instance);
		m_swapchain.CreatePresentationSurface(window);

		CreateDevice();

		m_swapchain.SetDevices(m_physicalDevice, m_device);
		m_swapchain.Create(width, height);

		CreateSyncObjects();
		CreateCommandPools();
		AllocateCommandBuffers();
		CreateTestRenderPass();
		CreateFramebuffers();
		CreateShaders();

		CreateDescriptors();

		CreateGraphicsPipelines();

		CreateTestVertexAndTriangleBuffers();

		RecordTestGraphicsCommands();

		return true;
	}

	void RendererVK::Cleanup()
	{
		WaitForDevice();

		DestroyTestVertexAndTriangleBuffers();

		DestroyDescriptors();

		DestroyGraphicsPipelines();
		DestroyShaders();
		DestroyFramebuffers();
		DestroyTestRenderPass();
		DestroyCommandPools();
		DestroySyncObjects();
		m_swapchain.Cleanup();
		DestroyDevice();
		m_swapchain.DestroyPresentationSurface();
		DestroyInstance();
	}

	void RendererVK::Update(float dt)
	{
		m_uboTest.m_testColor.x += dt * 0.0001f;
		if (m_uboTest.m_testColor.x > 1.0f)
			m_uboTest.m_testColor.x = 0.0f;
	}

	void RendererVK::Draw()
	{
		uint32_t imageIndex = m_swapchain.AcquireNextImage(m_acquireSemaphores[m_currentFrame]);


		// update uniform buffer
		UpdateBuffer(m_uboBuffers[imageIndex], sizeof(UBOTest), &m_uboTest);


		assert(imageIndex != UINT32_MAX);

		SubmitGraphicsQueue(imageIndex, m_currentFrame);

		m_swapchain.PresentQueue(m_graphicsQueue, imageIndex, m_renderSemaphores[m_currentFrame]);

		m_currentFrame = (m_currentFrame + 1) % s_maxFramesInFlight;
	}

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

	uint32_t RendererVK::FindDeviceQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags desiredCapabilities, bool queryPresentationSupport)
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
				if (queryPresentationSupport)
				{
					VkBool32 presentationSupported = VK_FALSE;
					VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_swapchain.m_presentationSurface, &presentationSupported);

					if (!presentationSupported)
						continue;
				}

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
			VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_swapchain.m_presentationSurface, &presentationSupported);

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

			// check queues support

			m_graphicsQueueFamily = FindDeviceQueueFamilyIndex(device, VK_QUEUE_GRAPHICS_BIT, true);

			if (m_graphicsQueueFamily != 0xFFFF)
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
		std::set<uint32_t> uniqueQueueFamilies = { m_graphicsQueueFamily };

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

		vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);

		return true;
	}

	void RendererVK::DestroyDevice()
	{
		vkDestroyDevice(m_device, nullptr);
	}

	bool RendererVK::CreateSyncObjects()
	{
		m_acquireSemaphores.resize(s_maxFramesInFlight);
		m_renderSemaphores.resize(s_maxFramesInFlight);
		m_fences.resize(m_swapchain.m_imageCount);

		VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = 0;

		for (int i = 0; i < s_maxFramesInFlight; ++i)
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
		createInfo.queueFamilyIndex = m_graphicsQueueFamily;

		VK_CHECK(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_graphicsCommandPool));

		return true;
	}

	void RendererVK::DestroyCommandPools()
	{
		vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
	}

	bool RendererVK::AllocateCommandBuffers()
	{
		m_graphicsCommandBuffers.resize(m_swapchain.m_imageCount);

		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.pNext = nullptr;
		allocateInfo.commandPool = m_graphicsCommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = (uint32_t) m_graphicsCommandBuffers.size();

		VK_CHECK(vkAllocateCommandBuffers(m_device, &allocateInfo, m_graphicsCommandBuffers.data()));

		return true;
	}

	void RendererVK::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkAccessFlags srcAccessMask = 0;
		VkAccessFlags dstAccessMask = 0;

		VkPipelineStageFlags srcStageMask;
		VkPipelineStageFlags dstStageMask;

		switch (oldLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			break;

		case VK_IMAGE_LAYOUT_GENERAL:
			srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;
		default:
			std::cout << "image transition FROM this type layout not implemented yet!" << std::endl;
			assert(0);

			break;
		}

		switch (newLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			break;
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

			break;
		case VK_IMAGE_LAYOUT_GENERAL:
			dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

			break;
		default:
			std::cout << "image transition TO this type layout not implemented yet!" << std::endl;
			assert(0);

			break;
		}

		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.pNext = nullptr;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		// subresource range
		barrier.subresourceRange.aspectMask = aspectFlags;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

		vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void RendererVK::RecordTestGraphicsCommands()
	{
		// test recording

		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO  };
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional: only relevant for secondary command buffers. It specifies which state to inherit from the calling primary command buffers

		for (int i = 0; i < m_graphicsCommandBuffers.size(); ++i)
		{
			auto commandBuffer = m_graphicsCommandBuffers[i];

			VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

#if 0

			VkClearColorValue clearColorValue = { 1.0, 0.0, 0.0, 1.0 };
			
			// transition the swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			
			TransitionImageLayout(commandBuffer, m_swapchain.m_images[i], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			
			VkImageSubresourceRange subresourceRange;
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 1;
			
			vkCmdClearColorImage(commandBuffer, m_swapchain.m_images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValue, 1, &subresourceRange);
			
			// transition the swapchain image to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			
			TransitionImageLayout(commandBuffer, m_swapchain.m_images[i], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

#else


			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_testRenderPass;
			renderPassInfo.framebuffer = m_swapchainFramebuffers[i];

			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = m_swapchain.m_imageExtent;

			VkClearValue clearColorValue;
			clearColorValue.color = { 0.3f, 0.3f, 0.3f, 1.0f };

			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColorValue;

			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_testGraphicsPipeline);

			VkBuffer vbs[] = { m_testVertexBuffer.m_buffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vbs, offsets);
			vkCmdBindIndexBuffer(commandBuffer, m_testIndexBuffer.m_buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_testGraphicsPipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);

			vkCmdPushConstants(commandBuffer, m_testGraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_pushConstantTestColor), &m_pushConstantTestColor);

			vkCmdDrawIndexed(commandBuffer, sizeof(m_testTriangleIndices) / sizeof(uint32_t), 1, 0, 0, 0);

			vkCmdEndRenderPass(commandBuffer);

#endif




			VK_CHECK(vkEndCommandBuffer(commandBuffer));
		}
	}

	bool RendererVK::CreateTestRenderPass()
	{
		VkAttachmentDescription attachmentDescriptions[1];
		attachmentDescriptions[0].flags = 0;
		attachmentDescriptions[0].format = m_swapchain.m_imageFormat;
		attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference attatchmentReferences[1];
		attatchmentReferences[0].attachment = 0;
		attatchmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescriptions[1];
		subpassDescriptions[0].flags = 0;
		subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptions[0].inputAttachmentCount = 0;
		subpassDescriptions[0].pInputAttachments = nullptr;
		subpassDescriptions[0].colorAttachmentCount = 1;
		subpassDescriptions[0].pColorAttachments = attatchmentReferences;
		subpassDescriptions[0].pResolveAttachments = 0;
		subpassDescriptions[0].pDepthStencilAttachment = nullptr;
		subpassDescriptions[0].preserveAttachmentCount = 0;
		subpassDescriptions[0].pPreserveAttachments = nullptr;

		VkSubpassDependency dependency;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dependencyFlags = 0;


		VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachmentDescriptions;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = subpassDescriptions;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		VK_CHECK(vkCreateRenderPass(m_device, &createInfo, nullptr, &m_testRenderPass));

		return true;
	}

	void RendererVK::DestroyTestRenderPass()
	{
		vkDestroyRenderPass(m_device, m_testRenderPass, nullptr);
	}

	bool RendererVK::CreateFramebuffers()
	{
		m_swapchainFramebuffers.resize(m_swapchain.m_imageCount);

		for (size_t i = 0; i < m_swapchainFramebuffers.size(); ++i)
		{
			VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.renderPass = m_testRenderPass;
			createInfo.attachmentCount = 1;
			createInfo.pAttachments = &m_swapchain.m_imageViews[i];
			createInfo.width = m_swapchain.m_imageExtent.width;
			createInfo.height = m_swapchain.m_imageExtent.height;
			createInfo.layers = 1;

			VK_CHECK(vkCreateFramebuffer(m_device, &createInfo, nullptr, &m_swapchainFramebuffers[i]));
		}

		return true;
	}

	void RendererVK::DestroyFramebuffers()
	{
		for (size_t i = 0; i < m_swapchainFramebuffers.size(); ++i)
		{
			vkDestroyFramebuffer(m_device, m_swapchainFramebuffers[i], nullptr);
		}
	}

	bool RendererVK::CreateShaderModuleFromFile(const char* fileName, VkShaderModule &shaderModule)
	{
		std::vector<char> shaderCode;
		
		if (!Utils::ReadFile(fileName, shaderCode))
			return false;

		VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.codeSize = shaderCode.size();
		createInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode.data());

		VK_CHECK(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule));

		return true;
	}

	bool RendererVK::CreateShaders()
	{
		bool result = true;

		// TODO: put common data/shader dir path in a variable or define
		result &= CreateShaderModuleFromFile("data/shaders/test.vert.spv", m_testVertexShader);
		result &= CreateShaderModuleFromFile("data/shaders/test.frag.spv", m_testFragmentShader);

		assert(result);

		return result;
	}

	void RendererVK::DestroyShaders()
	{
		vkDestroyShaderModule(m_device, m_testVertexShader, nullptr);
		vkDestroyShaderModule(m_device, m_testFragmentShader, nullptr);
	}

	bool RendererVK::CreateGraphicsPipelines()
	{
		VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2];
		// vertex
		shaderStageCreateInfos[0].sType = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		shaderStageCreateInfos[0].pNext = nullptr;
		shaderStageCreateInfos[0].flags = 0;
		shaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStageCreateInfos[0].module = m_testVertexShader;
		shaderStageCreateInfos[0].pName = "main";
		shaderStageCreateInfos[0].pSpecializationInfo = nullptr;
		// fragment
		shaderStageCreateInfos[1].sType = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		shaderStageCreateInfos[1].pNext = nullptr;
		shaderStageCreateInfos[1].flags = 0;
		shaderStageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStageCreateInfos[1].module = m_testFragmentShader;
		shaderStageCreateInfos[1].pName = "main";
		shaderStageCreateInfos[1].pSpecializationInfo = nullptr;

		VkVertexInputBindingDescription bindingDescription[1];
		bindingDescription[0].binding = 0;
		bindingDescription[0].stride = sizeof(TestVertex);
		bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription attributeDescription[2];
		// pos
		attributeDescription[0].location = 0; // shader input location
		attributeDescription[0].binding = 0; // vertex buffer binding
		attributeDescription[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescription[0].offset = offsetof(TestVertex, pos);
		// color
		attributeDescription[1].location = 1; // shader input location
		attributeDescription[1].binding = 0; // vertex buffer binding
		attributeDescription[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescription[1].offset = offsetof(TestVertex, color);

		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		vertexInputCreateInfo.pNext = nullptr;
		vertexInputCreateInfo.flags = 0;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = bindingDescription;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = 2;
		vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescription;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssemblyCreateInfo.pNext = nullptr;
		inputAssemblyCreateInfo.flags = 0;
		inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {0, 0, (float)m_swapchain.m_imageExtent.width, (float)m_swapchain.m_imageExtent.height, 0.0f, 1.0f};
		VkRect2D scissor = {0, 0, m_swapchain.m_imageExtent};

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportStateCreateInfo.pNext = nullptr;
		viewportStateCreateInfo.flags = 0;
		viewportStateCreateInfo.viewportCount = 1;
		viewportStateCreateInfo.pViewports = &viewport;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		rasterizationCreateInfo.pNext = nullptr;
		rasterizationCreateInfo.flags = 0;
		rasterizationCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationCreateInfo.depthBiasClamp = 0.0f;
		rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
		rasterizationCreateInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisampleCreateInfo.pNext = nullptr;
		multisampleCreateInfo.flags = 0;
		multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleCreateInfo.minSampleShading = 1.0f;
		multisampleCreateInfo.pSampleMask = nullptr;
		multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState blendAttachmentState;
		blendAttachmentState.blendEnable = VK_FALSE;
		blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		colorBlendCreateInfo.pNext = nullptr;
		colorBlendCreateInfo.flags = 0;
		colorBlendCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendCreateInfo.attachmentCount = 1;
		colorBlendCreateInfo.pAttachments = &blendAttachmentState;
		colorBlendCreateInfo.blendConstants[0] = 0.0f;
		colorBlendCreateInfo.blendConstants[1] = 0.0f;
		colorBlendCreateInfo.blendConstants[2] = 0.0f;
		colorBlendCreateInfo.blendConstants[3] = 0.0f;

		VkPushConstantRange pushConstantRange;
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(m_pushConstantTestColor);

		VkPipelineLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		layoutCreateInfo.pNext = nullptr;
		layoutCreateInfo.flags = 0;
		layoutCreateInfo.setLayoutCount = 1;
		layoutCreateInfo.pSetLayouts = &m_descriptorSetLayout;
		layoutCreateInfo.pushConstantRangeCount = 1;
		layoutCreateInfo.pPushConstantRanges = &pushConstantRange;

		VK_CHECK(vkCreatePipelineLayout(m_device, &layoutCreateInfo, nullptr, &m_testGraphicsPipelineLayout));

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		pipelineCreateInfo.pNext = nullptr;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = shaderStageCreateInfos;
		pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		pipelineCreateInfo.pTessellationState = nullptr;
		pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
		pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
		pipelineCreateInfo.pDepthStencilState = nullptr;
		pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
		pipelineCreateInfo.pDynamicState = nullptr;
		pipelineCreateInfo.layout = m_testGraphicsPipelineLayout;
		pipelineCreateInfo.renderPass = m_testRenderPass;
		pipelineCreateInfo.subpass = 0;
		// handle of a pipeline to derive from
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		// pass a valid index if the pipeline to derive from is in the same batch of pipelines passed to this vkCreateGraphicsPipelines call
		pipelineCreateInfo.basePipelineIndex = -1;

		VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_testGraphicsPipeline));

		return true;
	}

	void RendererVK::DestroyGraphicsPipelines()
	{
		vkDestroyPipelineLayout(m_device, m_testGraphicsPipelineLayout, nullptr);
		vkDestroyPipeline(m_device, m_testGraphicsPipeline, nullptr);
	}

	bool RendererVK::WaitForDevice()
	{
		VK_CHECK(vkDeviceWaitIdle(m_device));

		return true;
	}

	void RendererVK::SubmitGraphicsQueue(uint32_t imageIndex, int currentFrame)
	{
		VK_CHECK(vkWaitForFences(m_device, 1, &m_fences[imageIndex], VK_TRUE, UINT64_MAX));
		VK_CHECK(vkResetFences(m_device, 1, &m_fences[imageIndex]));

		VkSemaphore waitSemaphores[] = { m_acquireSemaphores[currentFrame] };
		VkSemaphore signalSemaphores[] = { m_renderSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
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

	void RendererVK::CreateTestVertexAndTriangleBuffers()
	{
		VkDeviceSize size = sizeof(m_testTriangleVerts);

		CreateBuffer(m_testVertexBuffer, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		UpdateBuffer(m_testVertexBuffer, size, m_testTriangleVerts);

		// index buffer

		size = sizeof(m_testTriangleIndices);

		CreateBuffer(m_testIndexBuffer, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		UpdateBuffer(m_testIndexBuffer, size, m_testTriangleIndices);
	}

	void RendererVK::DestroyTestVertexAndTriangleBuffers()
	{
		DestroyBuffer(m_testVertexBuffer);
		DestroyBuffer(m_testIndexBuffer);
	}

	bool RendererVK::CreateDescriptors()
	{
		// Create UBO

		m_uboBuffers.resize(m_swapchain.m_imageCount);

		for (size_t i = 0; i < m_uboBuffers.size(); ++i)
		{
			CreateBuffer(m_uboBuffers[i], sizeof(UBOTest), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			UpdateBuffer(m_uboBuffers[i], sizeof(UBOTest), &m_uboTest);
		}
		

		// Create Descriptor Pool
		VkDescriptorPoolSize poolSizes[1];
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = m_swapchain.m_imageCount;

		VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.maxSets = m_swapchain.m_imageCount;
		createInfo.poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
		createInfo.pPoolSizes = poolSizes;

		VK_CHECK(vkCreateDescriptorPool(m_device, &createInfo, nullptr, &m_descriptorPool));

		// Create Descriptor Set Layouts

		VkDescriptorSetLayoutBinding bindings[1];
		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		bindings[0].pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutCreateInfo.pNext = nullptr;
		layoutCreateInfo.flags = 0;
		layoutCreateInfo.bindingCount = 1;
		layoutCreateInfo.pBindings = bindings;

		VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutCreateInfo, nullptr, &m_descriptorSetLayout));

		// Create Descriptor Sets

		std::vector<VkDescriptorSetLayout> layouts(m_swapchain.m_imageCount, m_descriptorSetLayout);
		
		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.pNext = nullptr;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = (uint32_t) layouts.size();
		allocInfo.pSetLayouts = layouts.data();

		m_descriptorSets.resize(m_swapchain.m_imageCount);

		VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()));

		for(size_t i = 0; i < m_descriptorSets.size(); ++i)
		{
			VkDescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = m_uboBuffers[i].m_buffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UBOTest);

			VkWriteDescriptorSet descriptorWrites[1];
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].pNext = nullptr;
			descriptorWrites[0].dstSet = m_descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].pImageInfo = nullptr;
			descriptorWrites[0].pBufferInfo = &bufferInfo;
			descriptorWrites[0].pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(m_device, sizeof(descriptorWrites) / sizeof(VkWriteDescriptorSet), descriptorWrites, 0, nullptr);
		}

		return true;
	}

	void RendererVK::DestroyDescriptors()
	{
		for (size_t i = 0; i < m_uboBuffers.size(); ++i)
			DestroyBuffer(m_uboBuffers[i]);

		vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

		vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
	}

	uint32_t RendererVK::FindMemoryType(VkMemoryPropertyFlags requiredProperties, VkMemoryRequirements memoryRequirements, VkPhysicalDeviceMemoryProperties deviceMemoryProperties)
	{
		for (uint32_t type = 0; type < deviceMemoryProperties.memoryTypeCount; ++type)
		{
			if ((memoryRequirements.memoryTypeBits & (1 << type)) && ((deviceMemoryProperties.memoryTypes[type].propertyFlags & requiredProperties) == requiredProperties))
				return type;
		}

		return 0xFFFF;
	}

	bool RendererVK::CreateBuffer(BufferVK& buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties)
	{
		assert(buffer.m_buffer == VK_NULL_HANDLE);

		buffer.m_size = size;
		buffer.m_usage = usage;

		buffer.m_hasCpuAccess = memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		// if we are not using host memory, we need VK_BUFFER_USAGE_TRANSFER_DST_BIT so we can upload data to the GPU via staging buffers
		if (!buffer.m_hasCpuAccess)
			buffer.m_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.size = buffer.m_size;
		createInfo.usage = buffer.m_usage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;

		VK_CHECK(vkCreateBuffer(m_device, &createInfo, nullptr, &buffer.m_buffer));

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(m_device, buffer.m_buffer, &memoryRequirements);

		VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &deviceMemoryProperties);

		uint32_t memoryType = FindMemoryType(memoryProperties, memoryRequirements, deviceMemoryProperties);

		assert(memoryType != 0xFFFF);

		if (memoryType == 0xFFFF)
			return false;

		VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = memoryType;

		VK_CHECK(vkAllocateMemory(m_device, &allocateInfo, nullptr, &buffer.m_memory));

		assert(buffer.m_memory != VK_NULL_HANDLE);

		if (!buffer.m_memory)
			return false;

		vkBindBufferMemory(m_device, buffer.m_buffer, buffer.m_memory, 0);

		if (buffer.m_hasCpuAccess)
		{
			// leave it permanently mapped until destruction
			VK_CHECK(vkMapMemory(m_device, buffer.m_memory, 0, buffer.m_size, 0, &buffer.m_data));

			assert(buffer.m_data);

			if (!(memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			{
				VkMappedMemoryRange memoryRanges[1];
				memoryRanges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				memoryRanges[0].pNext = nullptr;
				memoryRanges[0].memory = buffer.m_memory;
				memoryRanges[0].offset = 0;
				memoryRanges[0].size = VK_WHOLE_SIZE;

				VK_CHECK(vkFlushMappedMemoryRanges(m_device, 1, memoryRanges));
			}
		}

		return true;
	}

	bool RendererVK::UpdateBuffer(BufferVK& buffer, VkDeviceSize size, void* data)
	{
		assert(size <= buffer.m_size);

		if (buffer.m_hasCpuAccess)
		{
			std::memcpy(buffer.m_data, data, size);

			return true;
		}

		// if we have no CPU access, we need to create a staging buffer and upload it to GPU

		if (!(buffer.m_usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT))
		{
			assert(!"Cannot upload data to device local memory via staging buffer. Usage needs to be VK_BUFFER_USAGE_TRANSFER_DST_BIT.");
			return false;
		}

		BufferVK stagingBuffer;
		CreateBuffer(stagingBuffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

		std::memcpy(stagingBuffer.m_data, data, size);

		// Copy staging buffer to device local memory

		VkCommandBuffer commandBuffer = BeginNewCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VkBufferCopy region;
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = size;

		vkCmdCopyBuffer(commandBuffer, stagingBuffer.m_buffer, buffer.m_buffer, 1, &region);

		SubmitCommandBufferAndWait(commandBuffer);

		vkFreeCommandBuffers(m_device, m_graphicsCommandPool, 1, &commandBuffer);

		DestroyBuffer(stagingBuffer);

		return true;
	}

	void RendererVK::DestroyBuffer(BufferVK& buffer)
	{
		vkFreeMemory(m_device, buffer.m_memory, nullptr);
		vkDestroyBuffer(m_device, buffer.m_buffer, nullptr);
	}

	VkCommandBuffer RendererVK::BeginNewCommandBuffer(VkCommandBufferUsageFlags usage)
	{
		VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.pNext = nullptr;
		allocInfo.commandPool = m_graphicsCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		VK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer));

		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = usage;
		beginInfo.pInheritanceInfo = nullptr;

		VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		return commandBuffer;
	}

	void RendererVK::SubmitCommandBufferAndWait(VkCommandBuffer commandBuffer)
	{
		VK_CHECK(vkEndCommandBuffer(commandBuffer));

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, nullptr));

		VK_CHECK(vkQueueWaitIdle(m_graphicsQueue));
	}

}