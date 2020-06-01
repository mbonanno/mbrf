#pragma once

#include "application.h"

namespace MBRF
{

class HelloTriangle : public Application
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
	bool CreateUniformBuffers();
	bool CreateGraphicsPipelines();

	void DestroyShaders();
	void DestroyUniformBuffers();
	void DestroyGraphicsPipelines();

	ShaderVK m_vertexShader;
	ShaderVK m_fragmentShader;

	ShaderVK m_testFragmentShader2;

	GraphicsPipelineVK m_graphicsPipeline;
	GraphicsPipelineVK m_testGraphicsPipeline2;

	struct Vertex
	{
		glm::vec3 m_pos;
		glm::vec4 m_color;
		glm::vec2 m_texcoord;
	};

	Vertex m_testCubeVerts[8] =
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

	VertexFormatVK m_vertexFormat;

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
