#pragma once

#include "deviceVK.h"
#include "pipelineVK.h"
#include "shaderVK.h"
#include "swapchainVK.h"
#include "uniformBufferVK.h"
#include "vertexBufferVK.h"
#include "vertexFormatVK.h"

namespace MBRF
{

class RendererVK
{
public:
	bool Init(GLFWwindow* window, uint32_t width, uint32_t height);
	void Cleanup();

	void Update(double dt);
	void Draw();

	void RequestSwapchainResize(uint32_t width, uint32_t height);

private:
	bool CreateBackBuffer();
	void CreateTextures();
	void CreateTestVertexAndTriangleBuffers();

	void DestroyBackBuffer();
	void DestroyTextures();
	void DestroyTestVertexAndTriangleBuffers();
	
	void DrawFrame(uint32_t currentFrameIndex);
	
	// TODO: create wrappers for the following stuff

	bool CreateShaders();
	bool CreateUniformBuffers();
	bool CreateGraphicsPipelines();

	void DestroyShaders();
	void DestroyUniformBuffers();
	void DestroyGraphicsPipelines();

	void ResizeSwapchain();

private:
	SwapchainVK m_swapchain;
	DeviceVK m_device;

	bool m_pendingSwapchainResize = false;
	uint32_t m_swapchainWidth = 0;
	uint32_t m_swapchainHeight = 0;

	static const int s_maxFramesInFlight = 2;

	struct BackBuffer
	{
		std::vector<FrameBufferVK> m_frameBuffers;
		TextureVK m_depthStencilBuffer;
	};

	BackBuffer m_backBuffer;

	ShaderVK m_testVertexShader;
	ShaderVK m_testFragmentShader;

	ShaderVK m_testFragmentShader2;

	GraphicsPipelineVK m_testGraphicsPipeline;
	GraphicsPipelineVK m_testGraphicsPipeline2;

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

	VertexBufferVK m_testVertexBuffer;
	IndexBufferVK m_testIndexBuffer;

	glm::vec4 m_pushConstantTestColor = glm::vec4(0, 1, 0, 1);

	// TODO: make sure of alignment requirements
	struct UBOTest
	{
		glm::mat4 m_mvpTransform;
		glm::vec4 m_testColor;
	};

	UBOTest m_uboTest = { glm::mat4(), {1, 0, 1, 1} };
	UBOTest m_uboTest2 = { glm::mat4(), {1, 0, 1, 1} };

	UniformBufferVK m_uboBuffers1;
	UniformBufferVK m_uboBuffers2;

	TextureVK m_testTexture;
	TextureVK m_testTexture2;
};

}
