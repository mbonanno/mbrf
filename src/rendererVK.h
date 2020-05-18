#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // use the Vulkan range of 0.0 to 1.0, instead of the -1 to 1.0 OpenGL range
#include "glm.hpp"

#include "deviceVK.h"
#include "swapchainVK.h"

namespace MBRF
{

class RendererVK
{
public:
	bool Init(GLFWwindow* window, uint32_t width, uint32_t height);
	void Cleanup();

	void Update(double dt);
	void Draw();

private:
	bool CreateDepthStencilBuffer(DeviceVK* device);
	bool CreateFramebuffers(DeviceVK* device);
	void CreateTextures(DeviceVK* device);
	void CreateTestVertexAndTriangleBuffers(DeviceVK* device);

	void DestroyDepthStencilBuffer(DeviceVK* device);
	void DestroyFramebuffers(DeviceVK* device);
	void DestroyTextures(DeviceVK* device);
	void DestroyTestVertexAndTriangleBuffers(DeviceVK* device);
	
	void DrawFrame();
	
	// TODO: create wrappers for the following stuff

	bool CreateShaders(DeviceVK* device);
	bool CreateDescriptors(DeviceVK* device);
	bool CreateGraphicsPipelines(DeviceVK* device);

	void DestroyShaders(DeviceVK* device);
	void DestroyDescriptors(DeviceVK* device);
	void DestroyGraphicsPipelines(DeviceVK* device);

private:
	SwapchainVK m_swapchain;
	DeviceVK m_device;

	static const int s_maxFramesInFlight = 2;




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
};

}