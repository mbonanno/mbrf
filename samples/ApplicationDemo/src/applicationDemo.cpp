#include "applicationDemo.h"

namespace MBRF
{

void ApplicationDemo::OnInit()
{
	CreateTextures();
	CreateTestVertexAndTriangleBuffers();

	CreateShaders();

	CreateGraphicsPipelines();
}

void ApplicationDemo::OnCleanup()
{
	DestroyGraphicsPipelines();
	DestroyShaders();

	DestroyTestVertexAndTriangleBuffers();
	DestroyTextures();
}

void ApplicationDemo::OnResize()
{
	DestroyGraphicsPipelines();
	CreateGraphicsPipelines();
}

void ApplicationDemo::OnUpdate(double dt)
{
	m_uboTest.m_testColor.x += (float)dt;
	if (m_uboTest.m_testColor.x > 1.0f)
		m_uboTest.m_testColor.x = 0.0f;

	m_testCubeRotation += (float)dt;

	FrameBufferVK* backBuffer = m_rendererVK.GetCurrentBackBuffer();

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), m_testCubeRotation * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), backBuffer->GetWidth() / float(backBuffer->GetHeight()), 0.1f, 10.0f);

	// Vulkan clip space has inverted Y and half Z.
	glm::mat4 clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);

	m_uboTest.m_mvpTransform = clip * proj * view * model;

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), m_testCubeRotation * glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	m_uboTest2.m_mvpTransform = clip * proj * view * model;
}

void ApplicationDemo::OnDraw()
{
	// TODO: store current swapchain FBO as a global
	FrameBufferVK* currentRenderTarget = m_rendererVK.GetCurrentBackBuffer();

	ContextVK* context = m_rendererVK.GetDevice()->GetCurrentGraphicsContext();

	context->BeginPass(currentRenderTarget);

	context->ClearRenderTarget(0, 0, currentRenderTarget->GetWidth(), currentRenderTarget->GetHeight(), { 0.3f, 0.3f, 0.3f, 1.0f }, { 1.0f, 0 });

	// TODO: need to write PSO wrapper. After replace this with context->SetPSO. Orset individual states + final ApplyState call?
	context->SetPipeline(&m_graphicsPipeline);

	context->SetVertexBuffer(&m_testVertexBuffer, 0);
	context->SetIndexBuffer(&m_testIndexBuffer, 0);

	// Draw first test cube

	context->SetUniformBuffer(m_rendererVK.GetDevice(), &m_uboTest, sizeof(UBOTest), 0);
	context->SetTexture(&m_testTexture, 0);

	context->CommitBindings(m_rendererVK.GetDevice());

	context->DrawIndexed(m_testIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	// Draw second test cube

	context->SetPipeline(&m_testGraphicsPipeline2);

	context->SetUniformBuffer(m_rendererVK.GetDevice(), &m_uboTest2, sizeof(UBOTest), 0);
	context->SetTexture(&m_testTexture2, 0);

	context->CommitBindings(m_rendererVK.GetDevice());

	context->DrawIndexed(m_testIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	context->EndPass();
}

void ApplicationDemo::CreateTextures()
{
	m_testTexture.LoadFromFile(m_rendererVK.GetDevice(), "../../data/textures/test.jpg");
	m_testTexture2.LoadFromFile(m_rendererVK.GetDevice(), "../../data/textures/test2.png");
}

bool ApplicationDemo::CreateShaders()
{
	bool result = true;

	// TODO: put common data/shader dir path in a variable or define
	result &= m_vertexShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/ApplicationDemo/test.vert.spv", SHADER_STAGE_VERTEX);
	result &= m_fragmentShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/ApplicationDemo/test.frag.spv", SHADER_STAGE_FRAGMENT);

	result &= m_fragmentShader2.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/ApplicationDemo/test2.frag.spv", SHADER_STAGE_FRAGMENT);

	assert(result);

	return result;
}

bool ApplicationDemo::CreateGraphicsPipelines()
{
	GraphicsPipelineDesc desc;
	desc.m_vertexFormat = &m_vertexFormat;
	desc.m_frameBuffer = m_rendererVK.GetCurrentBackBuffer();
	desc.m_shaders = { m_vertexShader, m_fragmentShader };
	desc.m_cullMode = CULL_MODE_BACK;

	m_graphicsPipeline.Create(m_rendererVK.GetDevice(), desc);

	desc.m_shaders = { m_vertexShader, m_fragmentShader2 };

	m_testGraphicsPipeline2.Create(m_rendererVK.GetDevice(), desc);

	return true;
}

void ApplicationDemo::CreateTestVertexAndTriangleBuffers()
{
	// vertex buffer

	uint32_t size = sizeof(m_testCubeVerts);

	m_testVertexBuffer.Create(m_rendererVK.GetDevice(), size, m_testCubeVerts);

	// index buffer

	size = sizeof(m_testCubeIndices);

	m_testIndexBuffer.Create(m_rendererVK.GetDevice(), size, size / sizeof(uint32_t), false, m_testCubeIndices);

	// vertex format

	m_vertexFormat.AddAttribute(VertexAttributeVK(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_pos), sizeof(Vertex::m_pos)));
	m_vertexFormat.AddAttribute(VertexAttributeVK(1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, m_color), sizeof(Vertex::m_color)));
	m_vertexFormat.AddAttribute(VertexAttributeVK(2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, m_texcoord), sizeof(Vertex::m_texcoord)));
}

void ApplicationDemo::DestroyShaders()
{
	m_vertexShader.Destroy(m_rendererVK.GetDevice());
	m_fragmentShader.Destroy(m_rendererVK.GetDevice());

	m_fragmentShader2.Destroy(m_rendererVK.GetDevice());
}

void ApplicationDemo::DestroyGraphicsPipelines()
{
	m_graphicsPipeline.Destroy(m_rendererVK.GetDevice());

	m_testGraphicsPipeline2.Destroy(m_rendererVK.GetDevice());
}

void ApplicationDemo::DestroyTestVertexAndTriangleBuffers()
{
	m_testVertexBuffer.Destroy(m_rendererVK.GetDevice());
	m_testIndexBuffer.Destroy(m_rendererVK.GetDevice());
}

void ApplicationDemo::DestroyTextures()
{
	m_testTexture.Destroy(m_rendererVK.GetDevice());
	m_testTexture2.Destroy(m_rendererVK.GetDevice());
}

}

int main(int argc, char **argv)
{
	MBRF::ApplicationDemo app;

	app.ParseCommandLineArguments(argc, argv);

	app.Run();

	return 0;
}