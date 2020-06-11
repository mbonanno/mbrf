#include "deviceVK.h"

#include "swapchainVK.h"
#include "utilsVK.h"
#include "utils.h"

#include "shaderCommon.h"

#include <iostream>
#include <set>

#include "glfw/glfw3.h"

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

// ------------------------------- FrameDataVK -------------------------------

bool FrameDataVK::Create(DeviceVK* device)
{
	VkDevice logicDevice = device->GetDevice();

	VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	VK_CHECK(vkCreateSemaphore(logicDevice, &semaphoreCreateInfo, nullptr, &m_acquireSemaphore));
	VK_CHECK(vkCreateSemaphore(logicDevice, &semaphoreCreateInfo, nullptr, &m_renderSemaphore));

	return true;
}

void FrameDataVK::Destroy(DeviceVK* device)
{
	VkDevice logicDevice = device->GetDevice();

	vkDestroySemaphore(logicDevice, m_acquireSemaphore, nullptr);
	vkDestroySemaphore(logicDevice, m_renderSemaphore, nullptr);

	m_acquireSemaphore = VK_NULL_HANDLE;
	m_renderSemaphore = VK_NULL_HANDLE;
}

// ------------------------------- DeviceVK -------------------------------

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

void DeviceVK::Init(SwapchainVK* swapchain, GLFWwindow* window, uint32_t width, uint32_t height, uint32_t maxFramesInFlight, bool enableValidation)
{
	m_currentFrame = 0;
	m_swapchain = swapchain;
	m_maxFramesInFlight = maxFramesInFlight;

	CreateInstance(enableValidation);

	m_swapchain->CreatePresentationSurface(this, window);
	CreateDevice();
	m_swapchain->Create(this, width, height);
	CreateCommandPools();
	CreateDescriptorSetLayouts();

	CreateFrameData();
	CreateGraphicsContexts();

	m_swapchainImageCount = m_swapchain->m_imageCount;
}

void DeviceVK::Cleanup()
{
	DestroyGraphicsContexts();
	DestroyFrameData();

	DestroyDescriptorSetLayouts();
	DestroyCommandPools();
	m_swapchain->Destroy(this);
	DestroyDevice();
	m_swapchain->DestroyPresentationSurface(this);
	DestroyInstance();
}

void DeviceVK::RecreateSwapchain(uint32_t width, uint32_t height)
{
	// cleanup old swapchann
	DestroyGraphicsContexts();
	m_swapchain->Destroy(this, true);

	// create new one
	m_swapchain->Create(this, width, height);
	CreateGraphicsContexts();
}

bool DeviceVK::CreateInstance(bool enableValidation)
{
	m_validationLayerEnabled = enableValidation;

	if (m_validationLayerEnabled)
		std::cout << "[DeviceVK::CreateInstance] Using Vulkan validation layer." << std::endl;

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
	if (enableValidation && !UtilsVK::CheckLayersSupport(validationLayers, availableLayers))
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

		if ( m_validationLayerEnabled && !UtilsVK::CheckLayersSupport(validationLayers, availableLayers))
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

	
bool DeviceVK::CreateFrameData()
{
	m_frameData.resize(m_maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.flags = 0;

	for (int i = 0; i < m_frameData.size(); ++i)
		m_frameData[i].Create(this);

	return true;
}

void DeviceVK::DestroyFrameData()
{
	for (int i = 0; i < m_frameData.size(); ++i)
		m_frameData[i].Destroy(this);
}

bool DeviceVK::CreateCommandPools()
{
	VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;  // command buffers can be reset, explicitly or implicitly (calling vkBeginCommandBuffer)
	createInfo.queueFamilyIndex = m_graphicsQueueFamily;

	VK_CHECK(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_graphicsCommandPool));

	return true;
}

void DeviceVK::DestroyCommandPools()
{
	vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
}

bool DeviceVK::CreateDescriptorSetLayouts()
{
	VkDescriptorSetLayoutBinding bindings[MAX_NUM_BINDINGS];
	int currentBinding = 0;

	// UBOs
	for (int i = 0; i < MAX_UNIFORM_BUFFER_SLOTS; ++i)
	{
		bindings[currentBinding].binding = UNIFORM_BUFFER_SLOT(i);
		bindings[currentBinding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[currentBinding].descriptorCount = 1;
		bindings[currentBinding].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // TODO: set all stages?
		bindings[currentBinding].pImmutableSamplers = nullptr;

		currentBinding++;
	}
	
	// Textures + Samplers TODO: create separate samplers?
	for (int i = 0; i < MAX_TEXTURE_SLOTS; ++i)
	{
		bindings[currentBinding].binding = TEXTURE_SLOT(i);
		bindings[currentBinding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		bindings[currentBinding].descriptorCount = 1;
		bindings[currentBinding].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[currentBinding].pImmutableSamplers = nullptr;

		currentBinding++;
	}

	// Storage Images
	for (int i = 0; i < MAX_STORAGE_IMAGE_SLOTS; ++i)
	{
		bindings[currentBinding].binding = STORAGE_IMAGE_SLOT(i);
		bindings[currentBinding].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		bindings[currentBinding].descriptorCount = 1;
		bindings[currentBinding].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		bindings[currentBinding].pImmutableSamplers = nullptr;

		currentBinding++;
	}

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutCreateInfo.pNext = nullptr;
	layoutCreateInfo.flags = 0;
	layoutCreateInfo.bindingCount = sizeof(bindings) / sizeof(VkDescriptorSetLayoutBinding);
	layoutCreateInfo.pBindings = bindings;

	VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutCreateInfo, nullptr, &m_descriptorSetLayout));

	return true;
}

void DeviceVK::DestroyDescriptorSetLayouts()
{
	vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
}

bool DeviceVK::CreateGraphicsContexts()
{
	m_graphicsContexts.resize(m_swapchain->m_imageCount);

	for (auto &graphicContext : m_graphicsContexts)
		graphicContext.Create(this);
	
	return true;
}

void DeviceVK::DestroyGraphicsContexts()
{
	for (auto &graphicContext : m_graphicsContexts)
		graphicContext.Destroy(this);
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
		srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		srcStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

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
		dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

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

bool DeviceVK::WaitForDevice()
{
	VK_CHECK(vkDeviceWaitIdle(m_device));

	return true;
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

bool DeviceVK::Present()
{
	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &m_currentFrameData->m_renderSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain->m_swapchain;
	presentInfo.pImageIndices = &m_currentImageIndex;
	presentInfo.pResults = nullptr;

	VkResult result = vkQueuePresentKHR(m_graphicsQueue, &presentInfo);

	return (result != VK_ERROR_OUT_OF_DATE_KHR);
}

bool DeviceVK::BeginFrame()
{
	m_currentFrameData = &m_frameData[m_currentFrame];

	m_currentImageIndex = m_swapchain->AcquireNextImage(this);

	if (m_swapchain->m_outOfDate)
		return false;

	assert(m_currentImageIndex != UINT32_MAX);

	m_currentGraphicsContext = &m_graphicsContexts[m_currentImageIndex];

	m_currentGraphicsContext->WaitForLastFrame(this);

	return true;
}

bool DeviceVK::EndFrame()
{
	m_currentGraphicsContext->Submit(m_graphicsQueue, m_currentFrameData->m_acquireSemaphore, m_currentFrameData->m_renderSemaphore);

	bool success = Present();

	// if presenting failed, it means that the swapchain is out of date and needs to be recreated
	if (!success)
		return false;

	m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;

	return true;
}

}