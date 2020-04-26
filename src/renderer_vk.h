#pragma once

#define GLFW_INCLUDE_VULKAN
#include "glfw/glfw3.h"

#include <assert.h>
#include <vector>

namespace MBRF
{
	class Utils
	{
	public:
		static bool ReadFile(const char* fileName, std::vector<char> &fileOut);
	};

	class RendererVK
	{
	public:
		bool CreateInstance(bool enableValidation);
		void DestroyInstance();

		bool CreatePresentationSurface(GLFWwindow* window);
		void DestroyPresentationSurface();

		bool CreateDevice();
		void DestroyDevice();

		bool CreateSwapchain(uint32_t width, uint32_t height);
		void DestroySwapchain();

		bool CreateSyncObjects(int maxFramesInFlight);
		void DestroySyncObjects();

		bool CreateCommandPools();
		void DestroyCommandPools();

		bool AllocateCommandBuffers();

		void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout);

		void RecordTestGraphicsCommands();

		bool CreateShaderModuleFromFile(const char* fileName, VkShaderModule &shaderModule);
		bool CreateShaders();
		void DestroyShaders();

		void SubmitGraphicsQueue(uint32_t imageIndex, int currentFrame);

		uint32_t AcquireNextSwapchainImage(int currentFrame);
		bool PresentSwapchainImage(uint32_t imageIndex, int currentFrame);

		bool WaitForDevice();

	private:
		bool CheckExtensionsSupport(const std::vector<const char*>& requiredExtensions, const std::vector<VkExtensionProperties>& availableExtensions);
		bool CheckLayersSupport(const std::vector<const char*>& requiredLayers, const std::vector<VkLayerProperties>& availableLayers);

		uint32_t FindDeviceQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags desiredCapabilities);
		uint32_t FindDevicePresentationQueueFamilyIndex(VkPhysicalDevice device);

	private:
		bool m_validationLayerEnabled;

		// vulkan (TODO: move in a wrapper)
		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debugMessenger;
		VkSurfaceKHR m_presentationSurface;

		VkPhysicalDevice m_physicalDevice;
		VkPhysicalDeviceProperties m_physicalDeviceProperties;
		VkPhysicalDeviceFeatures m_physicalDeviceFeatures;

		VkSwapchainKHR m_swapchain;
		std::vector<VkImage> m_swapchainImages;

		VkDevice m_device;
		
		uint32_t m_graphicQueueFamily;
		uint32_t m_presentationQueueFamily;

		VkQueue m_graphicsQueue;
		VkQueue m_presentationQueue;

		VkCommandPool m_graphicsCommandPool;

		VkShaderModule m_testVertexShader;
		VkShaderModule m_testFragmentShader;

		// TODO: move all the frame dependent objects in a frame data structure, and just use a frame data array?

		std::vector<VkCommandBuffer> m_graphicsCommandBuffers;

		std::vector<VkSemaphore> m_acquireSemaphores;
		std::vector<VkSemaphore> m_renderSemaphores;

		std::vector<VkFence> m_fences;
	};

}