#include "deviceVK.h"

#include "swapchainVK.h"
#include "utilsVK.h"
#include "utils.h"

#include <iostream>
#include <set>

#include "glfw/glfw3.h"
#include <gtc/matrix_transform.hpp>

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

void DeviceVK::Init(SwapchainVK* swapchain)
{
	m_swapchain = swapchain;
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

	
bool DeviceVK::CreateSyncObjects(int numFrames)
{
	m_acquireSemaphores.resize(numFrames);
	m_renderSemaphores.resize(numFrames);
	m_fences.resize(m_swapchain->m_imageCount);

	VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	for (int i = 0; i < numFrames; ++i)
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
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

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
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

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

void DeviceVK::ClearFramebufferAttachments(VkCommandBuffer commandBuffer, const FrameBufferVK& frameBuffer, int32_t x, int32_t y, uint32_t width, uint32_t height, VkClearValue clearValues[2])
{
	VkClearRect clearRect;
	clearRect.rect = { {x, y}, {width, height} };
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;

	std::vector<TextureViewVK> attachments = frameBuffer.GetAttachments();

	std::vector<VkClearAttachment> clearAttachments;
	clearAttachments.resize(attachments.size());

	for (size_t i = 0; i < attachments.size(); ++i)
	{
		VkImageAspectFlags aspectMask = attachments[i].GetAspectMask();

		clearAttachments[i].aspectMask = aspectMask;
		clearAttachments[i].colorAttachment = uint32_t(i);

		if (aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
			clearAttachments[i].clearValue.color = clearValues[0].color;
		else if ((aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) || aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)
			clearAttachments[i].clearValue.depthStencil = clearValues[1].depthStencil;
	}

	vkCmdClearAttachments(commandBuffer, uint32_t(clearAttachments.size()), clearAttachments.data(), 1, &clearRect);
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
		
		TransitionImageLayout(commandBuffer, m_swapchain->m_images[i], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_swapchainFramebuffers[i].GetRenderPass();
		renderPassInfo.framebuffer = m_swapchainFramebuffers[i].GetFrameBuffer();

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapchain->m_imageExtent;

		renderPassInfo.clearValueCount = 0;
		renderPassInfo.pClearValues = nullptr;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkClearValue clearValues[2];
		clearValues[0].color = { 0.3f, 0.3f, 0.3f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		ClearFramebufferAttachments(commandBuffer, m_swapchainFramebuffers[i], 0, 0, m_swapchain->m_imageExtent.width, m_swapchain->m_imageExtent.height, clearValues);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_testGraphicsPipeline);

		VkBuffer vbs[] = { m_testVertexBuffer.GetBuffer() };
		VkDeviceSize offsets[] = { 0 };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vbs, offsets);
		vkCmdBindIndexBuffer(commandBuffer, m_testIndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_testGraphicsPipelineLayout, 0, 1, &m_descriptorSets[i], 0, nullptr);

		vkCmdPushConstants(commandBuffer, m_testGraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_pushConstantTestColor), &m_pushConstantTestColor);

		vkCmdDrawIndexed(commandBuffer, sizeof(m_testCubeIndices) / sizeof(uint32_t), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

		TransitionImageLayout(commandBuffer, m_swapchain->m_images[i], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

#endif

		VK_CHECK(vkEndCommandBuffer(commandBuffer));
	}
}

bool DeviceVK::CreateFramebuffers()
{
	m_swapchainFramebuffers.resize(m_swapchain->m_imageCount);

	for (size_t i = 0; i < m_swapchainFramebuffers.size(); ++i)
	{
		std::vector<TextureViewVK> attachments = { m_swapchain->m_textureViews[i], m_depthTexture.GetView() };

		m_swapchainFramebuffers[i].Create(this, m_swapchain->m_imageExtent.width, m_swapchain->m_imageExtent.height, attachments);
	}

	return true;
}

void DeviceVK::DestroyFramebuffers()
{
	for (size_t i = 0; i < m_swapchainFramebuffers.size(); ++i)
		m_swapchainFramebuffers[i].Destroy(this);
}

bool DeviceVK::CreateShaderModuleFromFile(const char* fileName, VkShaderModule &shaderModule)
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
	pipelineCreateInfo.renderPass = m_swapchainFramebuffers[0].GetRenderPass();
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

	m_testVertexBuffer.Create(this, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_testVertexBuffer.Update(this, size, m_testCubeVerts);

	// index buffer

	size = sizeof(m_testCubeIndices);

	m_testIndexBuffer.Create(this, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_testIndexBuffer.Update(this, size, m_testCubeIndices);
}

void DeviceVK::DestroyTestVertexAndTriangleBuffers()
{
	m_testVertexBuffer.Destroy(this);
	m_testIndexBuffer.Destroy(this);
}

bool DeviceVK::CreateDepthStencilBuffer()
{
	// TODO: check VK_FORMAT_D24_UNORM_S8_UINT format availability!
	m_depthTexture.Create(this, VK_FORMAT_D24_UNORM_S8_UINT, m_swapchain->m_imageExtent.width, m_swapchain->m_imageExtent.height, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	// Transition (not really needed, we could just set the initial layout to VK_IMAGE_LAYOUT_UNDEFINED in the subpass?)

	m_depthTexture.TransitionImageLayoutAndSubmit(this, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	return true;
}

void DeviceVK::DestroyDepthStencilBuffer()
{
	m_depthTexture.Destroy(this);
}

bool DeviceVK::CreateDescriptors()
{
	// Create UBO

	m_uboBuffers.resize(m_swapchain->m_imageCount);

	for (size_t i = 0; i < m_uboBuffers.size(); ++i)
	{
		m_uboBuffers[i].Create(this, sizeof(UBOTest), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		m_uboBuffers[i].Update(this, sizeof(UBOTest), &m_uboTest);
	}


	// Create Descriptor Pool
	VkDescriptorPoolSize poolSizes[2];
	// UBOs
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = m_swapchain->m_imageCount;
	// Texture + Samplers TODO: create separate samplers?
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
	// Texture + Samplers TODO: create separate samplers?
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
		VkDescriptorBufferInfo bufferInfo = m_uboBuffers[i].GetDescriptor();

		VkDescriptorImageInfo imageInfo = m_testTexture.GetDescriptor();

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
		m_uboBuffers[i].Destroy(this);

	vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

	vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
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

void DeviceVK::SubmitCommandBufferAndWait(VkCommandBuffer commandBuffer, bool freeCommandBuffer)
{
	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VK_CHECK(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, nullptr));

	VK_CHECK(vkQueueWaitIdle(m_graphicsQueue));

	if (freeCommandBuffer)
		vkFreeCommandBuffers(m_device, m_graphicsCommandPool, 1, &commandBuffer);
}



void DeviceVK::CreateTextures()
{
	m_testTexture.LoadFromFile(this, "data/textures/test.jpg");
}

void DeviceVK::DestroyTextures()
{
	m_testTexture.Destroy(this);
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
	uint32_t imageIndex = m_swapchain->AcquireNextImage(this, m_acquireSemaphores[currentFrame]);


	// update uniform buffer
	m_uboBuffers[imageIndex].Update(this, sizeof(UBOTest), &m_uboTest);


	assert(imageIndex != UINT32_MAX);

	SubmitGraphicsQueue(imageIndex, currentFrame);

	Present(imageIndex, currentFrame);
}

}