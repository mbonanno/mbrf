#pragma once

#include "bufferVK.h"
#include "commonVK.h"
#include "frameBufferVK.h"
#include "textureVK.h"

#include "glfw/glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // use the Vulkan range of 0.0 to 1.0, instead of the -1 to 1.0 OpenGL range
#include "glm.hpp"

namespace MBRF
{

class SwapchainVK;

// TODO: remove any scene specific data, resources etc + any Update, Draw functionality from here

struct FrameDataVK
{
	VkSemaphore m_acquireSemaphore = VK_NULL_HANDLE;
	VkSemaphore m_renderSemaphore = VK_NULL_HANDLE;
};

// TODO: add anything related to command buffers recording and submission to this class
class ContextVK
{
public:
	void Submit(DeviceVK* device, VkQueue queue);

	VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
	VkFence m_fence = VK_NULL_HANDLE;
	VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

	// TODO: add a BufferVK to use as scratch uniforms buffer? Could be a dynamic offset one?
};

class DeviceVK
{
public:
	// TODO: create a device init info struct parameter? Also, remove swapchain dependencies in DeviceVK?
	void Init(SwapchainVK* swapchain, GLFWwindow* window, uint32_t width, uint32_t height, uint32_t maxFramesInFlight);
	void Cleanup();

	// TODO: nothing to do with the device...move this stuff out ASAP
	void Update(double dt);
	void Draw();

	void BeginFrame();
	void EndFrame();

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

	bool CreateFramebuffers();
	void DestroyFramebuffers();

	bool CreateShaderModuleFromFile(const char* fileName, VkShaderModule &shaderModule);
	bool CreateShaders();
	void DestroyShaders();

	bool CreateGraphicsPipelines();
	void DestroyGraphicsPipelines();

	bool WaitForDevice();

	void CreateTestVertexAndTriangleBuffers();
	void DestroyTestVertexAndTriangleBuffers();

	bool CreateDepthStencilBuffer();
	void DestroyDepthStencilBuffer();

	bool CreateDescriptors();
	void DestroyDescriptors();

	VkInstance& GetInstance() { return m_instance; };
	VkPhysicalDevice& GetPhysicalDevice() { return m_physicalDevice; };
	VkDevice& GetDevice() { return m_device; };

	FrameDataVK* GetCurrentFrameData() const { return m_currentFrameData; };

//private:

	uint32_t FindDeviceQueueFamilyIndex(VkPhysicalDevice device, VkQueueFlags desiredCapabilities, bool queryPresentationSupport);
	uint32_t FindDevicePresentationQueueFamilyIndex(VkPhysicalDevice device);


	VkCommandBuffer BeginNewCommandBuffer(VkCommandBufferUsageFlags usage);
	void SubmitCommandBufferAndWait(VkCommandBuffer commandBuffer, bool freeCommandBuffer);

	void CreateTextures();
	void DestroyTextures();

	bool Present();

	// clearColors[0] = color, clearColors[1] = depth+stencil
	void ClearFramebufferAttachments(VkCommandBuffer commandBuffer, const FrameBufferVK& frameBuffer, int32_t x, int32_t y, uint32_t width, uint32_t height, VkClearValue clearValues[2]);

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

	std::vector<FrameBufferVK> m_swapchainFramebuffers;

	VkShaderModule m_testVertexShader;
	VkShaderModule m_testFragmentShader;

	VkPipelineLayout m_testGraphicsPipelineLayout;
	VkPipeline m_testGraphicsPipeline;

	TextureVK m_depthTexture;

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

	VkDescriptorPool m_descriptorPool;
	VkDescriptorSetLayout m_descriptorSetLayout;

	std::vector<FrameDataVK> m_frameData;
	FrameDataVK* m_currentFrameData = nullptr;

	std::vector<ContextVK> m_graphicContexts;

	uint32_t m_currentFrame = UINT32_MAX;
	uint32_t m_currentImageIndex = UINT32_MAX;
	uint32_t m_maxFramesInFlight = 2;
};

}
