#pragma once

#include "application.h"

namespace MBRF
{

class PostProcessing: public Application
{
	void OnInit();
	void OnCleanup();
	void OnResize();
	void OnUpdate(double dt);
	void OnDraw();

	void CreateTextures();
	void CreateTestVertexAndTriangleBuffers();

	void DestroyTextures();
	void DestroyTestVertexAndTriangleBuffers();

	bool CreateShaders();
	bool CreateGraphicsPipelines();

	void DestroyShaders();
	void DestroyGraphicsPipelines();

	ShaderVK m_offscreenVertexShader;
	ShaderVK m_offscreenFragmentShader;

	ShaderVK m_computeShader;

	ShaderVK m_quadVertexShader;
	ShaderVK m_quadFragmentShader;

	GraphicsPipelineVK m_offscreenPipeline;
	GraphicsPipelineVK m_postProcPipeline;

	ComputePipelineVK m_computePipeline;

	struct VertexPosUV
	{
		glm::vec3 m_pos;
		glm::vec2 m_texcoord;
	};

	VertexPosUV m_testCubeVerts[8] =
	{
		// top
		{{-0.5, -0.5,  0.5}, {0, 1}},
		{{ 0.5, -0.5,  0.5}, {1, 1}},
		{{ 0.5,  0.5,  0.5}, {1, 0}},
		{{-0.5,  0.5,  0.5}, {0, 0}},
		// bottom
		{{-0.5, -0.5, -0.5}, {0, 1}},
		{{ 0.5, -0.5, -0.5}, {1, 1}},
		{{ 0.5,  0.5, -0.5}, {1, 0}},
		{{-0.5,  0.5, -0.5 }, {0, 0}}
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

	VertexPosUV m_quadVerts[4] =
	{
		{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }},
		{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }},
		{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }},
		{ { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }}
	};

	uint32_t m_quadIndices[6] = { 0,1,2, 2,3,0 };

	VertexFormatVK m_vertexFormatPosUV;

	float m_cubeRotation = 0;

	VertexBufferVK m_cubeVertexBuffer;
	IndexBufferVK m_cubeIndexBuffer;

	VertexBufferVK m_quadVertexBuffer;
	IndexBufferVK m_quadIndexBuffer;

	glm::vec4 m_pushConstantTestColor = glm::vec4(0, 1, 0, 1);

	// TODO: make sure of alignment requirements
	struct SceneUniforms
	{
		glm::mat4 m_mvpTransform;
	};

	SceneUniforms m_sceneUniforms = { glm::mat4() };

	const float m_nearPlane = 0.1f;
	const float m_farPlane = 10.0f;

	TextureVK m_renderTarget;
	TextureVK m_offscreenDepthStencil;

	TextureVK m_computeTarget;

	TextureVK m_sceneTexture;
	TextureVK m_vignetteTexture;

	FrameBufferVK m_offscreenFramebuffer;
};

}
