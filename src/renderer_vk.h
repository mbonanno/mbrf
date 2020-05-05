#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // use the Vulkan range of 0.0 to 1.0, instead of the -1 to 1.0 OpenGL range
#include "glm.hpp"

#include "swapchain_vk.h"

namespace MBRF
{
	class Utils
	{
	public:
		static bool ReadFile(const char* fileName, std::vector<char> &fileOut);
	};

	struct BufferVK
	{
		VkBuffer m_buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		VkDeviceSize m_size = 0;
		VkBufferUsageFlags m_usage;
		void* m_data = nullptr;

		bool m_hasCpuAccess = false;
	};

	class RendererVK
	{
	public:
		bool Init(GLFWwindow* window, uint32_t width, uint32_t height);
		void Cleanup();

		void Update(double dt);
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

		void CreateTestVertexAndTriangleBuffers();
		void DestroyTestVertexAndTriangleBuffers();

		bool CreateDepthStencilBuffer();
		void DestroyDepthStencilBuffer();

		bool CreateDescriptors();
		void DestroyDescriptors();

	private:
		bool CheckExtensionsSupport(const std::vector<const char*>& requiredExtensions, const std::vector<VkExtensionProperties>& availableExtensions);
		bool CheckLayersSupport(const std::vector<const char*>& requiredLayers, const std::vector<VkLayerProperties>& availableLayers);

		uint32_t FindDeviceQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags desiredCapabilities, bool queryPresentationSupport);
		uint32_t FindDevicePresentationQueueFamilyIndex(VkPhysicalDevice device);

		uint32_t FindMemoryType(VkMemoryPropertyFlags requiredProperties, VkMemoryRequirements memoryRequirements, VkPhysicalDeviceMemoryProperties deviceMemoryProperties);

		// TODO: move back all the buffer creation stuff in a separate BufferVK class
		bool CreateBuffer(BufferVK& buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
		void DestroyBuffer(BufferVK& buffer);

		bool UpdateBuffer(BufferVK& buffer, VkDeviceSize size, void* data);

		VkCommandBuffer BeginNewCommandBuffer(VkCommandBufferUsageFlags usage);
		void SubmitCommandBufferAndWait(VkCommandBuffer commandBuffer);

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

		VkImage m_depthImage;
		VkDeviceMemory m_depthMemory;
		VkImageView m_depthImageView;
		VkFormat m_depthFormat;

		struct TestVertex
		{
			glm::vec3 pos;
			glm::vec4 color;
		};

		TestVertex m_testTriangleVerts[3] =
		{
			{{0.0, -0.5, 0.0f}, {1.0, 0.0, 0.0, 1.0}},
			{{0.5, 0.5, 0.0f}, {0.0, 1.0, 0.0, 1.0}},
			{{-0.5, 0.5, 0.0f}, {0.0, 0.0, 1.0, 1.0}}
		};

		uint32_t m_testTriangleIndices[3] = { 0, 1, 2 };

		TestVertex m_testCubeVerts[8] =
		{
			// front
			{{-0.5, -0.5,  0.5}, {1.0, 0.0, 0.0, 1.0}},
			{{ 0.5, -0.5,  0.5}, {0.0, 1.0, 0.0, 1.0}},
			{{ 0.5,  0.5,  0.5}, {0.0, 0.0, 1.0, 1.0}},
			{{-0.5,  0.5,  0.5}, {1.0, 1.0, 1.0, 1.0}},
			// back
			{{-0.5, -0.5, -0.5}, {1.0, 0.0, 0.0, 1.0}},
			{{ 0.5, -0.5, -0.5}, {0.0, 1.0, 0.0, 1.0}},
			{{ 0.5,  0.5, -0.5}, {0.0, 0.0, 1.0, 1.0}},
			{{-0.5,  0.5, -0.5 }, {1.0, 1.0, 1.0, 1.0}}
		};

		uint32_t m_testCubeIndices[36] = 
		{
			// front
			0, 1, 2,
			2, 3, 0,
			// right
			1, 5, 6,
			6, 2, 1,
			// back
			7, 6, 5,
			5, 4, 7,
			// left
			4, 0, 3,
			3, 7, 4,
			// bottom
			4, 5, 1,
			1, 0, 4,
			// top
			3, 2, 6,
			6, 7, 3
		};

		float m_testCubeRotation = 0;

		BufferVK m_testVertexBuffer;
		BufferVK m_testIndexBuffer;

		glm::vec4 m_pushConstantTestColor = glm::vec4(0, 1, 0, 1);

		struct UBOTest
		{
			glm::mat4 m_mvpTransform;
			glm::vec4 m_testColor;
		};

		UBOTest m_uboTest = { glm::mat4(), {1, 0, 1, 1} };

		std::vector<BufferVK> m_uboBuffers;

		// TODO: move all the frame dependent objects in a frame data structure, and just use a frame data array?

		std::vector<VkCommandBuffer> m_graphicsCommandBuffers;

		std::vector<VkSemaphore> m_acquireSemaphores;
		std::vector<VkSemaphore> m_renderSemaphores;

		std::vector<VkFence> m_fences;

		VkDescriptorPool m_descriptorPool;
		VkDescriptorSetLayout m_descriptorSetLayout;
		std::vector<VkDescriptorSet> m_descriptorSets;
	};

}