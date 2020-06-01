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

	void CreateTestVertexAndTriangleBuffers();
	void DestroyTestVertexAndTriangleBuffers();

	bool CreateShaders();
	bool CreateGraphicsPipelines();

	void DestroyShaders();
	void DestroyGraphicsPipelines();

	ShaderVK m_vertexShader;
	ShaderVK m_fragmentShader;

	GraphicsPipelineVK m_graphicsPipeline;

	struct Vertex
	{
		glm::vec3 m_pos;
		glm::vec4 m_color;
	};

	Vertex m_triangleVerts[3] =
	{
		{{0.0, -0.5, 0.0f}, {1.0, 0.0, 0.0, 1.0}},
		{{0.5, 0.5, 0.0f}, {0.0, 1.0, 0.0, 1.0}},
		{{-0.5, 0.5, 0.0f}, {0.0, 0.0, 1.0, 1.0}}
	};

	VertexFormatVK m_vertexFormat;

	uint32_t m_triangleIndices[3] = { 0, 1, 2 };

	VertexBufferVK m_testVertexBuffer;
	IndexBufferVK m_testIndexBuffer;
};

}
