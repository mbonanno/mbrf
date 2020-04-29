#pragma once

#include "swapchain_vk.h"

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
		bool Init(GLFWwindow* window, uint32_t width, uint32_t height);
		void Cleanup();

		void Draw();

	private:
		bool CreateInstance(bool enableValidation);
		void DestroyInstance();

		bool CreateDevice();
		void DestroyDevice();

		bool CreateSyncObjects();
		void DestroySyncObjects();

		bool CreateCommandPools();
		void DestroyCommandPools();

		bool AllocateCommandBuffers();

		void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout);

		void RecordTestGraphicsCommands();

		bool CreateTestRenderPass();
		void DestroyTestRenderPass();

		bool CreateFramebuffers();
		void DestroyFramebuffers();

		bool CreateShaderModuleFromFile(const char* fileName, VkShaderModule &shaderModule);
		bool CreateShaders();
		void DestroyShaders();

		bool CreateGraphicsPipelines();
		void DestroyGraphicsPipelines();

		void SubmitGraphicsQueue(uint32_t imageIndex, int currentFrame);

		bool WaitForDevice();

	private:
		bool CheckExtensionsSupport(const std::vector<const char*>& requiredExtensions, const std::vector<VkExtensionProperties>& availableExtensions);
		bool CheckLayersSupport(const std::vector<const char*>& requiredLayers, const std::vector<VkLayerProperties>& availableLayers);

		uint32_t FindDeviceQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags desiredCapabilities, bool queryPresentationSupport);
		uint32_t FindDevicePresentationQueueFamilyIndex(VkPhysicalDevice device);

	private:
		bool m_validationLayerEnabled;

		uint32_t m_currentFrame;
		static const int s_maxFramesInFlight = 2;

		// vulkan (TODO: move in a wrapper)
		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debugMessenger;

		VkPhysicalDevice m_physicalDevice;
		VkPhysicalDeviceProperties m_physicalDeviceProperties;
		VkPhysicalDeviceFeatures m_physicalDeviceFeatures;

		SwapchainVK m_swapchain;

		VkDevice m_device;
		
		uint32_t m_graphicsQueueFamily;

		VkQueue m_graphicsQueue;

		VkCommandPool m_graphicsCommandPool;

		VkRenderPass m_testRenderPass;
		std::vector<VkFramebuffer> m_swapchainFramebuffers;

		VkShaderModule m_testVertexShader;
		VkShaderModule m_testFragmentShader;

		VkPipelineLayout m_testGraphicsPipelineLayout;
		VkPipeline m_testGraphicsPipeline;

		// TODO: move all the frame dependent objects in a frame data structure, and just use a frame data array?

		std::vector<VkCommandBuffer> m_graphicsCommandBuffers;

		std::vector<VkSemaphore> m_acquireSemaphores;
		std::vector<VkSemaphore> m_renderSemaphores;

		std::vector<VkFence> m_fences;
	};

}