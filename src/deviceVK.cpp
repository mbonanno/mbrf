#include "deviceVK.h"

#include "utilsVK.h"
#include "swapchainVK.h"

#include <iostream>
#include <set>

#include "glfw/glfw3.h"
#include <gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

// ------------------------------- DeviceVK -------------------------------

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

void DeviceVK::Init(SwapchainVK* swapchain, int numFrames)
{
	m_swapchain = swapchain;
	m_numFrames = numFrames;
}

bool DeviceVK::CreateInstance(bool enableValidation)
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
	if (!UtilsVK::CheckExtensionsSupport(requiredExtensions, availableExtensions))
		return false;

	uint32_t layerCount;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

	std::vector<VkLayerProperties> availableLayers(layerCount);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

	// check validation layer support
	if (!UtilsVK::CheckLayersSupport(validationLayers, availableLayers))
		return false;

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

void DeviceVK::DestroyInstance()
{
	if (m_validationLayerEnabled)
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);

	vkDestroyInstance(m_instance, nullptr);
}

uint32_t DeviceVK::FindDeviceQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags desiredCapabilities, bool queryPresentationSupport)
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
				VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_swapchain->m_presentationSurface, &presentationSupported);

				if (!presentationSupported)
					continue;
			}

			queueFamilyIndex = index;

			break;
		}
	}

	return queueFamilyIndex;
}

uint32_t DeviceVK::FindDevicePresentationQueueFamilyIndex(VkPhysicalDevice device)
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
		VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, index, m_swapchain->m_presentationSurface, &presentationSupported);

		if (presentationSupported)
		{
			queueFamilyIndex = index;
			break;
		}
	}

	return queueFamilyIndex;
}

bool DeviceVK::CreateDevice()
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

		if (!UtilsVK::CheckExtensionsSupport(requiredExtensions, availableExtensions))
			continue;

		// check validation layer support

		uint32_t layerCount;
		VK_CHECK(vkEnumerateDeviceLayerProperties(device, &layerCount, nullptr));

		std::vector<VkLayerProperties> availableLayers(layerCount);
		VK_CHECK(vkEnumerateDeviceLayerProperties(device, &layerCount, availableLayers.data()));

		if (!UtilsVK::CheckLayersSupport(validationLayers, availableLayers))
			continue;

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

	for (auto queueFamily : uniqueQueueFamilies)
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

void DeviceVK::DestroyDevice()
{
	vkDestroyDevice(m_device, nullptr);
}

	
bool DeviceVK::CreateSyncObjects()
{
	m_acquireSemaphores.resize(m_numFrames);
	m_renderSemaphores.resize(m_numFrames);
	m_fences.resize(m_swapchain->m_imageCount);

	VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	for (int i = 0; i < m_numFrames; ++i)
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

void DeviceVK::DestroySyncObjects()
{
	for (int i = 0; i < m_acquireSemaphores.size(); ++i)
	{
		vkDestroySemaphore(m_device, m_acquireSemaphores[i], nullptr);
		vkDestroySemaphore(m_device, m_renderSemaphores[i], nullptr);
	}

	for (int i = 0; i < m_fences.size(); ++i)
		vkDestroyFence(m_device, m_fences[i], nullptr);
}

bool DeviceVK::CreateCommandPools()
{
	VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = 0; // optional
	createInfo.queueFamilyIndex = m_graphicsQueueFamily;

	VK_CHECK(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_graphicsCommandPool));

	return true;
}

void DeviceVK::DestroyCommandPools()
{
	vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
}

bool DeviceVK::AllocateCommandBuffers()
{
	m_graphicsCommandBuffers.resize(m_swapchain->m_imageCount);

	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.pNext = nullptr;
	allocateInfo.commandPool = m_graphicsCommandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = (uint32_t)m_graphicsCommandBuffers.size();

	VK_CHECK(vkAllocateCommandBuffers(m_device, &allocateInfo, m_graphicsCommandBuffers.data()));

	return true;
}

void DeviceVK::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout)
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
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

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

void DeviceVK::RecordTestGraphicsCommands()
{
	// test recording

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional: only relevant for secondary command buffers. It specifies which state to inherit from the calling primary command buffers

	for (int i = 0; i < m_graphicsCommandBuffers.size(); ++i)
	{
		auto commandBuffer = m_graphicsCommandBuffers[i];

		VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

#if 0

		VkClearColorValue clearColorValues = { 1.0, 0.0, 0.0, 1.0 };

		// transition the swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL

		TransitionImageLayout(commandBuffer, m_swapchain->m_images[i], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkImageSubresourceRange subresourceRange;
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.layerCount = 1;

		vkCmdClearColorImage(commandBuffer, m_swapchain->m_images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValues, 1, &subresourceRange);

		// transition the swapchain image to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR

		TransitionImageLayout(commandBuffer, m_swapchain->m_images[i], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

#else


		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_testRenderPass;
		renderPassInfo.framebuffer = m_swapchainFramebuffers[i];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapchain->m_imageExtent;

		VkClearValue clearColorValues[2];
		clearColorValues[0].color = { 0.3f, 0.3f, 0.3f, 1.0f };
		clearColorValues[1].depthStencil = { 1.0f, 0 };

		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearColorValues;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_testGraphicsPipeline);

		VkBuffer vbs[] = { m_testVertexBuffer.m_buffer };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vbs, offsets);
		vkCmdBindIndexBuffer(commandBuffer, m_testIndexBuffer.m_buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_testGraphicsPipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);

		vkCmdPushConstants(commandBuffer, m_testGraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_pushConstantTestColor), &m_pushConstantTestColor);

		vkCmdDrawIndexed(commandBuffer, sizeof(m_testCubeIndices) / sizeof(uint32_t), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

#endif




		VK_CHECK(vkEndCommandBuffer(commandBuffer));
	}
}

bool DeviceVK::CreateTestRenderPass()
{
	VkAttachmentDescription attachmentDescriptions[2];
	// Color
	attachmentDescriptions[0].flags = 0;
	attachmentDescriptions[0].format = m_swapchain->m_imageFormat;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// Depth
	attachmentDescriptions[1].flags = 0;
	attachmentDescriptions[1].format = m_depthTexture.m_format;
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef;
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef;
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescriptions[1];
	subpassDescriptions[0].flags = 0;
	subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[0].inputAttachmentCount = 0;
	subpassDescriptions[0].pInputAttachments = nullptr;
	subpassDescriptions[0].colorAttachmentCount = 1;
	subpassDescriptions[0].pColorAttachments = &colorAttachmentRef;
	subpassDescriptions[0].pResolveAttachments = 0;
	subpassDescriptions[0].pDepthStencilAttachment = &depthAttachmentRef;
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
	createInfo.attachmentCount = sizeof(attachmentDescriptions) / sizeof(VkAttachmentDescription);
	createInfo.pAttachments = attachmentDescriptions;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = subpassDescriptions;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(m_device, &createInfo, nullptr, &m_testRenderPass));

	return true;
}

void DeviceVK::DestroyTestRenderPass()
{
	vkDestroyRenderPass(m_device, m_testRenderPass, nullptr);
}

bool DeviceVK::CreateFramebuffers()
{
	m_swapchainFramebuffers.resize(m_swapchain->m_imageCount);

	for (size_t i = 0; i < m_swapchainFramebuffers.size(); ++i)
	{
		VkImageView attachments[] = { m_swapchain->m_imageViews[i], m_depthTexture.m_view.m_imageView };

		VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.renderPass = m_testRenderPass;
		createInfo.attachmentCount = sizeof(attachments) / sizeof(VkImageView);
		createInfo.pAttachments = attachments;
		createInfo.width = m_swapchain->m_imageExtent.width;
		createInfo.height = m_swapchain->m_imageExtent.height;
		createInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(m_device, &createInfo, nullptr, &m_swapchainFramebuffers[i]));
	}

	return true;
}

void DeviceVK::DestroyFramebuffers()
{
	for (size_t i = 0; i < m_swapchainFramebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(m_device, m_swapchainFramebuffers[i], nullptr);
	}
}

bool DeviceVK::CreateShaderModuleFromFile(const char* fileName, VkShaderModule &shaderModule)
{
	std::vector<char> shaderCode;

	if (!UtilsVK::ReadFile(fileName, shaderCode))
		return false;

	VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<uint32_t*>(shaderCode.data());

	VK_CHECK(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule));

	return true;
}

bool DeviceVK::CreateShaders()
{
	bool result = true;

	// TODO: put common data/shader dir path in a variable or define
	result &= CreateShaderModuleFromFile("data/shaders/test.vert.spv", m_testVertexShader);
	result &= CreateShaderModuleFromFile("data/shaders/test.frag.spv", m_testFragmentShader);

	assert(result);

	return result;
}

void DeviceVK::DestroyShaders()
{
	vkDestroyShaderModule(m_device, m_testVertexShader, nullptr);
	vkDestroyShaderModule(m_device, m_testFragmentShader, nullptr);
}

bool DeviceVK::CreateGraphicsPipelines()
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

	VkVertexInputAttributeDescription attributeDescription[3];
	// pos
	attributeDescription[0].location = 0; // shader input location
	attributeDescription[0].binding = 0; // vertex buffer binding
	attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription[0].offset = offsetof(TestVertex, pos);
	// color
	attributeDescription[1].location = 1; // shader input location
	attributeDescription[1].binding = 0; // vertex buffer binding
	attributeDescription[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescription[1].offset = offsetof(TestVertex, color);
	// texcoord
	attributeDescription[2].location = 2; // shader input location
	attributeDescription[2].binding = 0; // vertex buffer binding
	attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescription[2].offset = offsetof(TestVertex, texcoord);

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputCreateInfo.pNext = nullptr;
	vertexInputCreateInfo.flags = 0;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = bindingDescription;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = sizeof(attributeDescription) / sizeof(VkVertexInputAttributeDescription);
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescription;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyCreateInfo.pNext = nullptr;
	inputAssemblyCreateInfo.flags = 0;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = { 0, 0, (float)m_swapchain->m_imageExtent.width, (float)m_swapchain->m_imageExtent.height, 0.0f, 1.0f };
	VkRect2D scissor = { 0, 0, m_swapchain->m_imageExtent };

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
	rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationCreateInfo.depthBiasClamp = 0.0f;
	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationCreateInfo.lineWidth = 1.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilStateCreateInfo.pNext = nullptr;
	depthStencilStateCreateInfo.flags = 0;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {};
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

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
	pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
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

void DeviceVK::DestroyGraphicsPipelines()
{
	vkDestroyPipelineLayout(m_device, m_testGraphicsPipelineLayout, nullptr);
	vkDestroyPipeline(m_device, m_testGraphicsPipeline, nullptr);
}

bool DeviceVK::WaitForDevice()
{
	VK_CHECK(vkDeviceWaitIdle(m_device));

	return true;
}

void DeviceVK::SubmitGraphicsQueue(uint32_t imageIndex, int currentFrame)
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

void DeviceVK::CreateTestVertexAndTriangleBuffers()
{
	VkDeviceSize size = sizeof(m_testCubeVerts);

	CreateBuffer(m_testVertexBuffer, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	UpdateBuffer(m_testVertexBuffer, size, m_testCubeVerts);

	// index buffer

	size = sizeof(m_testCubeIndices);

	CreateBuffer(m_testIndexBuffer, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	UpdateBuffer(m_testIndexBuffer, size, m_testCubeIndices);
}

void DeviceVK::DestroyTestVertexAndTriangleBuffers()
{
	DestroyBuffer(m_testVertexBuffer);
	DestroyBuffer(m_testIndexBuffer);
}

bool DeviceVK::CreateDepthStencilBuffer()
{
	// TODO: check VK_FORMAT_D24_UNORM_S8_UINT format availability!
	CreateTexture(m_depthTexture, VK_FORMAT_D24_UNORM_S8_UINT, m_swapchain->m_imageExtent.width, m_swapchain->m_imageExtent.height, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	// Transition (not really needed, we could just set the initial layout to VK_IMAGE_LAYOUT_UNDEFINED in the subpass?)

	TransitionImageLayout(m_depthTexture, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	return true;
}

void DeviceVK::DestroyDepthStencilBuffer()
{
	DestroyTexture(m_depthTexture);
}

bool DeviceVK::CreateDescriptors()
{
	// Create UBO

	m_uboBuffers.resize(m_swapchain->m_imageCount);

	for (size_t i = 0; i < m_uboBuffers.size(); ++i)
	{
		CreateBuffer(m_uboBuffers[i], sizeof(UBOTest), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		UpdateBuffer(m_uboBuffers[i], sizeof(UBOTest), &m_uboTest);
	}


	// Create Descriptor Pool
	VkDescriptorPoolSize poolSizes[2];
	// UBOs
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = m_swapchain->m_imageCount;
	// Texture + Samplers TODO: create separate samplers!
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = m_swapchain->m_imageCount;

	VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.maxSets = m_swapchain->m_imageCount;
	createInfo.poolSizeCount = sizeof(poolSizes) / sizeof(VkDescriptorPoolSize);
	createInfo.pPoolSizes = poolSizes;

	VK_CHECK(vkCreateDescriptorPool(m_device, &createInfo, nullptr, &m_descriptorPool));

	// Create Descriptor Set Layouts

	VkDescriptorSetLayoutBinding bindings[2];
	// UBOs
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].pImmutableSamplers = nullptr;
	// Texture + Samplers TODO: create separate samplers!
	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[1].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutCreateInfo.pNext = nullptr;
	layoutCreateInfo.flags = 0;
	layoutCreateInfo.bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding);
	layoutCreateInfo.pBindings = bindings;

	VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutCreateInfo, nullptr, &m_descriptorSetLayout));

	// Create Descriptor Sets

	std::vector<VkDescriptorSetLayout> layouts(m_swapchain->m_imageCount, m_descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = (uint32_t)layouts.size();
	allocInfo.pSetLayouts = layouts.data();

	m_descriptorSets.resize(m_swapchain->m_imageCount);

	VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()));

	for (size_t i = 0; i < m_descriptorSets.size(); ++i)
	{
		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = m_uboBuffers[i].m_buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UBOTest);

		VkDescriptorImageInfo imageInfo;
		imageInfo.sampler = m_testSampler;
		imageInfo.imageView = m_testTexture.m_view.m_imageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet descriptorWrites[2];
		// UBOs
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
		// Texture + Samplers
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].pNext = nullptr;
		descriptorWrites[1].dstSet = m_descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].pImageInfo = &imageInfo;
		descriptorWrites[1].pBufferInfo = nullptr;
		descriptorWrites[1].pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(m_device, sizeof(descriptorWrites) / sizeof(VkWriteDescriptorSet), descriptorWrites, 0, nullptr);
	}

	return true;
}

void DeviceVK::DestroyDescriptors()
{
	for (size_t i = 0; i < m_uboBuffers.size(); ++i)
		DestroyBuffer(m_uboBuffers[i]);

	vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

	vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
}

uint32_t DeviceVK::FindMemoryType(VkMemoryPropertyFlags requiredProperties, VkMemoryRequirements memoryRequirements, VkPhysicalDeviceMemoryProperties deviceMemoryProperties)
{
	for (uint32_t type = 0; type < deviceMemoryProperties.memoryTypeCount; ++type)
	{
		if ((memoryRequirements.memoryTypeBits & (1 << type)) && ((deviceMemoryProperties.memoryTypes[type].propertyFlags & requiredProperties) == requiredProperties))
			return type;
	}

	return 0xFFFF;
}

bool DeviceVK::CreateBuffer(BufferVK& buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties)
{
	assert(buffer.m_buffer == VK_NULL_HANDLE);

	buffer.m_size = size;
	buffer.m_usage = usage;

	buffer.m_hasCpuAccess = memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

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

	VK_CHECK(vkBindBufferMemory(m_device, buffer.m_buffer, buffer.m_memory, 0));

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

bool DeviceVK::UpdateBuffer(BufferVK& buffer, VkDeviceSize size, void* data)
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

void DeviceVK::DestroyBuffer(BufferVK& buffer)
{
	vkFreeMemory(m_device, buffer.m_memory, nullptr);
	vkDestroyBuffer(m_device, buffer.m_buffer, nullptr);

	buffer.m_buffer = VK_NULL_HANDLE;
	buffer.m_memory = VK_NULL_HANDLE;
}

bool DeviceVK::CreateTexture(TextureVK& texture, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mips, VkImageUsageFlags usage,
	VkImageType type, VkImageLayout initialLayout, VkMemoryPropertyFlags memoryProperty, VkSampleCountFlagBits sampleCount, VkImageTiling tiling)
{
	texture.m_imageType = type;
	texture.m_format = format;
	texture.m_width = width;
	texture.m_height = height;
	texture.m_depth = depth;
	texture.m_mips = mips;
	texture.m_sampleCount = sampleCount;
	texture.m_tiling = tiling;
	texture.m_usage = usage;
	texture.m_currentLayout = initialLayout;

	VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.imageType = type;
	createInfo.format = format;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = depth;
	createInfo.mipLevels = mips;
	createInfo.arrayLayers = 1;
	createInfo.samples = sampleCount;
	createInfo.tiling = tiling;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.initialLayout = initialLayout;

	VK_CHECK(vkCreateImage(m_device, &createInfo, nullptr, &texture.m_image));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(m_device, texture.m_image, &memoryRequirements);

	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &deviceMemoryProperties);

	VkMemoryPropertyFlags memoryProperties = memoryProperty;

	uint32_t memoryType = FindMemoryType(memoryProperties, memoryRequirements, deviceMemoryProperties);

	assert(memoryType != 0xFFFF);

	if (memoryType == 0xFFFF)
		return false;

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.pNext = nullptr;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryType;

	VK_CHECK(vkAllocateMemory(m_device, &allocateInfo, nullptr, &texture.m_memory));

	assert(texture.m_memory != VK_NULL_HANDLE);

	if (!texture.m_memory)
		return false;

	VK_CHECK(vkBindImageMemory(m_device, texture.m_image, texture.m_memory, 0));

	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;


	CreateTextureView(texture.m_view, texture, aspectMask);

	return true;
}

void DeviceVK::DestroyTexture(TextureVK& texture)
{
	DestroyTextureView(texture.m_view);

	vkFreeMemory(m_device, texture.m_memory, nullptr);
	vkDestroyImage(m_device, texture.m_image, nullptr);

	texture.m_image = VK_NULL_HANDLE;
	texture.m_memory = VK_NULL_HANDLE;
}

void DeviceVK::TransitionImageLayout(TextureVK& texture, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = BeginNewCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	TransitionImageLayout(commandBuffer, texture.m_image, aspectFlags, oldLayout, newLayout);

	SubmitCommandBufferAndWait(commandBuffer);

	vkFreeCommandBuffers(m_device, m_graphicsCommandPool, 1, &commandBuffer);

	texture.m_currentLayout = newLayout;
}

bool DeviceVK::CreateTextureView(TextureViewVK& textureView, const TextureVK& texture, VkImageAspectFlags aspectMask, VkImageViewType viewType, uint32_t baseMip, uint32_t mipCount)
{
	textureView.m_viewType = viewType;
	textureView.m_aspectMask = aspectMask;
	textureView.m_baseMip = baseMip;
	textureView.m_mipCount = mipCount;

	VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCreateInfo.pNext = nullptr;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = texture.m_image;
	imageViewCreateInfo.viewType = viewType;
	imageViewCreateInfo.format = texture.m_format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	// subresource range
	imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	imageViewCreateInfo.subresourceRange.baseMipLevel = baseMip;
	imageViewCreateInfo.subresourceRange.levelCount = mipCount;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	VK_CHECK(vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &textureView.m_imageView));

	return true;
}

void DeviceVK::DestroyTextureView(TextureViewVK& textureView)
{
	vkDestroyImageView(m_device, textureView.m_imageView, nullptr);

	textureView.m_imageView = VK_NULL_HANDLE;
}

VkCommandBuffer DeviceVK::BeginNewCommandBuffer(VkCommandBufferUsageFlags usage)
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

void DeviceVK::SubmitCommandBufferAndWait(VkCommandBuffer commandBuffer)
{
	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, nullptr));

	VK_CHECK(vkQueueWaitIdle(m_graphicsQueue));
}

void DeviceVK::LoadTextureFromFile(TextureVK& texture, const char* fileName)
{
	int texWidth, texHeight, texChannels;
	//stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	stbi_uc* pixels = stbi_load(fileName, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	CreateTexture(texture, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, 1, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	// upload image pixels to the GPU

	UpdateTexture(texture, texWidth, texHeight, 1, 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pixels);

	stbi_image_free(pixels);
}

// NOTE: this only updates mip0. TODO: handle subresources properly!
bool DeviceVK::UpdateTexture(TextureVK& texture, uint32_t width, uint32_t height, uint32_t depth, uint32_t bpp, VkImageLayout newLayout, void* data)
{
	// TODO: check that the size we are trying to copy is less than the texture size?

	if (!(texture.m_usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
	{
		assert(!"Cannot upload data to device local memory via staging buffer. Usage needs to be VK_BUFFER_USAGE_TRANSFER_DST_BIT.");
		return false;
	}

	VkDeviceSize size = width * height * depth * bpp;

	TransitionImageLayout(texture, texture.m_view.m_aspectMask, texture.m_currentLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	BufferVK stagingBuffer;
	CreateBuffer(stagingBuffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	std::memcpy(stagingBuffer.m_data, data, size);

	VkCommandBuffer commandBuffer = BeginNewCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VkBufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = texture.m_view.m_aspectMask;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, depth };

	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.m_buffer, texture.m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	SubmitCommandBufferAndWait(commandBuffer);
	vkFreeCommandBuffers(m_device, m_graphicsCommandPool, 1, &commandBuffer);

	TransitionImageLayout(m_testTexture, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newLayout);

	DestroyBuffer(stagingBuffer);

	return true;
}

void DeviceVK::CreateTexturesAndSamplers()
{
	LoadTextureFromFile(m_testTexture, "data/textures/test.jpg");

	// create sampler

	VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	// mipmapping
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;

	VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_testSampler));
}

void DeviceVK::DestroyTexturesAndSamplers()
{
	DestroyTexture(m_testTexture);

	vkDestroySampler(m_device, m_testSampler, nullptr);
}

bool DeviceVK::Present(uint32_t imageIndex, uint32_t currentFrame)
{
	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_renderSemaphores[currentFrame];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain->m_swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	VK_CHECK(vkQueuePresentKHR(m_graphicsQueue, &presentInfo));

	return true;
}

void DeviceVK::Update(double dt)
{
	m_uboTest.m_testColor.x += (float)dt;
	if (m_uboTest.m_testColor.x > 1.0f)
		m_uboTest.m_testColor.x = 0.0f;

	m_testCubeRotation += (float)dt;

	glm::mat4 model = glm::rotate(glm::mat4(1.0f), m_testCubeRotation * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), m_swapchain->m_imageExtent.width / (float)m_swapchain->m_imageExtent.height, 0.1f, 10.0f);

	// Vulkan clip space has inverted Y and half Z.
	glm::mat4 clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);

	m_uboTest.m_mvpTransform = clip * proj * view * model;
}

void DeviceVK::Draw(uint32_t currentFrame)
{
	uint32_t imageIndex = m_swapchain->AcquireNextImage(m_acquireSemaphores[currentFrame]);


	// update uniform buffer
	UpdateBuffer(m_uboBuffers[imageIndex], sizeof(UBOTest), &m_uboTest);


	assert(imageIndex != UINT32_MAX);

	SubmitGraphicsQueue(imageIndex, currentFrame);

	Present(imageIndex, currentFrame);
}

}