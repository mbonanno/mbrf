#include "helloTriangle.h"

namespace MBRF
{

void HelloTriangle::OnInit()
{
	CreateTestVertexAndTriangleBuffers();

	CreateShaders();

	CreateGraphicsPipelines();
}

void HelloTriangle::OnCleanup()
{
	DestroyGraphicsPipelines();
	DestroyShaders();

	DestroyTestVertexAndTriangleBuffers();
}

void HelloTriangle::OnResize()
{
	DestroyGraphicsPipelines();
	CreateGraphicsPipelines();
}

void HelloTriangle::OnUpdate(double dt)
{

}

void HelloTriangle::OnDraw()
{
	FrameBufferVK* currentRenderTarget = m_rendererVK.GetCurrentBackBuffer();

	ContextVK* context = m_rendererVK.GetDevice()->GetCurrentGraphicsContext();
	VkCommandBuffer commandBuffer = context->m_commandBuffer;

	context->BeginPass(currentRenderTarget);

	context->ClearRenderTarget(0, 0, currentRenderTarget->GetWidth(), currentRenderTarget->GetHeight(), { 0.3f, 0.3f, 0.3f, 1.0f }, { 1.0f, 0 });

	context->SetPipeline(&m_graphicsPipeline);

	context->SetVertexBuffer(&m_testVertexBuffer, 0);

	context->SetIndexBuffer(&m_testIndexBuffer, 0);

	context->DrawIndexed(m_testIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	context->EndPass();
}

bool HelloTriangle::CreateShaders()
{
	bool result = true;

	// TODO: put common data/shader dir path in a variable or define
	result &= m_vertexShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/HelloTriangle/helloTriangle.vert.spv", SHADER_STAGE_VERTEX);
	result &= m_fragmentShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/HelloTriangle/helloTriangle.frag.spv", SHADER_STAGE_FRAGMENT);

	assert(result);

	return result;
}

bool HelloTriangle::CreateGraphicsPipelines()
{
	std::vector<ShaderVK> shaders = { m_vertexShader, m_fragmentShader };
	m_graphicsPipeline.Create(m_rendererVK.GetDevice(), &m_vertexFormat, m_rendererVK.GetCurrentBackBuffer(), shaders, false);

	return true;
}

void HelloTriangle::CreateTestVertexAndTriangleBuffers()
{
	// vertex buffer

	uint32_t size = sizeof(m_triangleVerts);

	m_testVertexBuffer.Create(m_rendererVK.GetDevice(), size, m_triangleVerts);

	// index buffer

	size = sizeof(m_triangleIndices);

	m_testIndexBuffer.Create(m_rendererVK.GetDevice(), size, size / sizeof(uint32_t), false, m_triangleIndices);

	// vertex format

	m_vertexFormat.AddAttribute(VertexAttributeVK(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_pos), sizeof(Vertex::m_pos)));
	m_vertexFormat.AddAttribute(VertexAttributeVK(1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, m_color), sizeof(Vertex::m_color)));
}

void HelloTriangle::DestroyShaders()
{
	m_vertexShader.Destroy(m_rendererVK.GetDevice());
	m_fragmentShader.Destroy(m_rendererVK.GetDevice());
}

void HelloTriangle::DestroyGraphicsPipelines()
{
	m_graphicsPipeline.Destroy(m_rendererVK.GetDevice());
}

void HelloTriangle::DestroyTestVertexAndTriangleBuffers()
{
	m_testVertexBuffer.Destroy(m_rendererVK.GetDevice());
	m_testIndexBuffer.Destroy(m_rendererVK.GetDevice());
}

}

int main(int argc, char **argv)
{
	MBRF::HelloTriangle app;

	app.ParseCommandLineArguments(argc, argv);

	app.Run();

	return 0;
}