#pragma once

#include "application.h"

namespace MBRF
{

class GPUPathTracing : public Application
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

	ShaderVK m_computeShader;

	ShaderVK m_quadVertexShader;
	ShaderVK m_quadFragmentShader;

	GraphicsPipelineVK m_postProcPipeline;

	ComputePipelineVK m_computePipeline;

	struct VertexPosUV
	{
		glm::vec3 m_pos;
		glm::vec2 m_texcoord;
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

	TextureVK m_computeTarget;
};

}
