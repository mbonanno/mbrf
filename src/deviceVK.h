#pragma once

#include "commonVK.h"
#include "contextVK.h"

#include "glfw/glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // use the Vulkan range of 0.0 to 1.0, instead of the -1 to 1.0 OpenGL range
#include "glm.hpp"

namespace MBRF
{

class SwapchainVK;

// TODO: remove any scene specific data, resources etc + any Update, Draw functionality from here

class FrameDataVK
{
public:
	bool Create(DeviceVK* device);
	void Destroy(DeviceVK* device);

	VkSemaphore m_acquireSemaphore = VK_NULL_HANDLE;
	VkSemaphore m_renderSemaphore = VK_NULL_HANDLE;
};

class DeviceVK
{
public:
	// TODO: create a device init info struct parameter? Also, remove swapchain dependencies in DeviceVK?
	void Init(SwapchainVK* swapchain, GLFWwindow* window, uint32_t width, uint32_t height, uint32_t maxFramesInFlight, bool enableValidation);
	void Cleanup();

	void RecreateSwapchain(uint32_t width, uint32_t height);

	// if BeginFrame or EndFrame return false, it means that the swapchain/not optimal is out of date and needs to be recreated
	bool BeginFrame();
	bool EndFrame();

	bool CreateInstance(bool enableValidation);
	void DestroyInstance();

	bool CreateDevice();
	void DestroyDevice();

	bool CreateFrameData();
	void DestroyFrameData();

	bool CreateCommandPools();
	void DestroyCommandPools();

	bool CreateDescriptorSetLayouts();
	void DestroyDescriptorSetLayouts();

	bool CreateGraphicsContexts();
	void DestroyGraphicsContexts();

	void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout);

	bool WaitForDevice();

	VkCommandBuffer BeginNewCommandBuffer(VkCommandBufferUsageFlags usage);
	void SubmitCommandBufferAndWait(VkCommandBuffer commandBuffer, bool freeCommandBuffer);

	bool Present();

	uint32_t FindDeviceQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags desiredCapabilities, bool queryPresentationSupport);
	uint32_t FindDevicePresentationQueueFamilyIndex(VkPhysicalDevice device);

	VkInstance GetInstance() { return m_instance; };
	VkPhysicalDevice GetPhysicalDevice() { return m_physicalDevice; };
	VkDevice GetDevice() { return m_device; };

	VkDescriptorSetLayout GetDescriptorSetLayout() { return m_descriptorSetLayout; };

	VkCommandPool GetGraphicsCommandPool() { return m_graphicsCommandPool; };
	FrameDataVK* GetCurrentFrameData() const { return m_currentFrameData; };


	VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const { return m_physicalDeviceProperties; };
	

	ContextVK* GetCurrentGraphicsContext() { return m_currentGraphicsContext; };

//private:
	std::vector<FrameDataVK> m_frameData;
	FrameDataVK* m_currentFrameData = nullptr;

	std::vector<ContextVK> m_graphicsContexts;
	ContextVK* m_currentGraphicsContext = nullptr;

	uint32_t m_swapchainImageCount = 0;
	uint32_t m_currentFrame = 0;
	uint32_t m_currentImageIndex = 0;

	uint32_t m_maxFramesInFlight = 2;

private:
	bool m_validationLayerEnabled;

	SwapchainVK* m_swapchain;

	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;

	VkPhysicalDevice m_physicalDevice;
	VkPhysicalDeviceProperties m_physicalDeviceProperties;
	VkPhysicalDeviceFeatures m_physicalDeviceFeatures;

	VkDevice m_device;

	uint32_t m_graphicsQueueFamily;

	VkQueue m_graphicsQueue;

	VkCommandPool m_graphicsCommandPool;

	VkDescriptorSetLayout m_descriptorSetLayout;
};

}
