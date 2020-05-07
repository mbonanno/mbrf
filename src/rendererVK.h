#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // use the Vulkan range of 0.0 to 1.0, instead of the -1 to 1.0 OpenGL range
#include "glm.hpp"

#include "swapchainVK.h"

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

	struct TextureViewVK
	{
		VkImageView m_imageView = VK_NULL_HANDLE;

		VkImageViewType m_viewType;
		VkImageAspectFlags m_aspectMask;
		uint32_t m_baseMip;
		uint32_t m_mipCount;
	};

	struct TextureVK
	{
		VkImage m_image = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;

		TextureViewVK m_view;

		VkImageType m_imageType;
		VkFormat m_format;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		uint32_t m_depth = 0;
		uint32_t m_mips = 1;

		VkSampleCountFlagBits m_sampleCount;
		VkImageTiling m_tiling;
		VkImageUsageFlags m_usage;
		VkImageLayout m_currentLayout;
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

		bool CreateTexture(TextureVK& texture, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mips = 1, VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
						 VkImageType type = VK_IMAGE_TYPE_2D, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, VkMemoryPropertyFlags memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						 VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);
		void DestroyTexture(TextureVK& texture);

		bool UpdateTexture(TextureVK& texture, uint32_t width, uint32_t height, uint32_t depth, uint32_t bpp, VkImageLayout newLayout, void* data);

		void TransitionImageLayout(TextureVK& texture, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout);

		bool CreateTextureView(TextureViewVK& textureView, const TextureVK& texture, VkImageAspectFlags aspectMask, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t baseMip = 0, uint32_t mipCount = 1);
		void DestroyTextureView(TextureViewVK& textureView);


		VkCommandBuffer BeginNewCommandBuffer(VkCommandBufferUsageFlags usage);
		void SubmitCommandBufferAndWait(VkCommandBuffer commandBuffer);

		void LoadTextureFromFile(TextureVK& texture, const char* fileName);
		void CreateTexturesAndSamplers();
		void DestroyTexturesAndSamplers();

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

		TextureVK m_depthImage;

		struct TestVertex
		{
			glm::vec3 pos;
			glm::vec4 color;
			glm::vec2 texcoord;
		};

		TestVertex m_testTriangleVerts[3] =
		{
			{{0.0, -0.5, 0.0f}, {1.0, 0.0, 0.0, 1.0}, {0, 1}},
			{{0.5, 0.5, 0.0f}, {0.0, 1.0, 0.0, 1.0}, {1, 0}},
			{{-0.5, 0.5, 0.0f}, {0.0, 0.0, 1.0, 1.0}, {0, 0}}
		};

		uint32_t m_testTriangleIndices[3] = { 0, 1, 2 };

		TestVertex m_testCubeVerts[8] =
		{
			// top
			{{-0.5, -0.5,  0.5}, {1.0, 0.0, 0.0, 1.0}, {0, 1}},
			{{ 0.5, -0.5,  0.5}, {0.0, 1.0, 0.0, 1.0}, {1, 1}},
			{{ 0.5,  0.5,  0.5}, {0.0, 0.0, 1.0, 1.0}, {1, 0}},
			{{-0.5,  0.5,  0.5}, {1.0, 1.0, 1.0, 1.0}, {0, 0}},
			// bottom
			{{-0.5, -0.5, -0.5}, {1.0, 0.0, 0.0, 1.0}, {0, 1}},
			{{ 0.5, -0.5, -0.5}, {0.0, 1.0, 0.0, 1.0}, {1, 1}},
			{{ 0.5,  0.5, -0.5}, {0.0, 0.0, 1.0, 1.0}, {1, 0}},
			{{-0.5,  0.5, -0.5 }, {1.0, 1.0, 1.0, 1.0}, {0, 0}}
		};

		uint32_t m_testCubeIndices[36] = 
		{
			// top
			0, 1, 2,
			2, 3, 0,
			// right
			1, 5, 6,
			6, 2, 1,
			// bottom
			7, 6, 5,
			5, 4, 7,
			// left
			4, 0, 3,
			3, 7, 4,
			// back
			4, 5, 1,
			1, 0, 4,
			// front
			3, 2, 6,
			6, 7, 3
		};

		float m_testCubeRotation = 0;

		BufferVK m_testVertexBuffer;
		BufferVK m_testIndexBuffer;

		glm::vec4 m_pushConstantTestColor = glm::vec4(0, 1, 0, 1);

		// TODO: make sure of alignment requirements
		struct UBOTest
		{
			glm::mat4 m_mvpTransform;
			glm::vec4 m_testColor;
		};

		UBOTest m_uboTest = { glm::mat4(), {1, 0, 1, 1} };

		std::vector<BufferVK> m_uboBuffers;

		TextureVK m_testTexture;
		VkSampler m_testSampler;

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