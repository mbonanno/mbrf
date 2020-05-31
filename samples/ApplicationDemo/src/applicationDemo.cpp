#include "applicationDemo.h"

namespace MBRF
{

void ApplicationDemo::OnInit()
{
	CreateTextures();
	CreateTestVertexAndTriangleBuffers();

	CreateShaders();
	CreateUniformBuffers();

	CreateGraphicsPipelines();
}

void ApplicationDemo::OnCleanup()
{
	DestroyGraphicsPipelines();
	DestroyUniformBuffers();
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
	// update uniform buffer (TODO: move in ContextVK?)
	m_uboBuffers1.UpdateCurrentBuffer(m_rendererVK.GetDevice(), &m_uboTest);
	m_uboBuffers2.UpdateCurrentBuffer(m_rendererVK.GetDevice(), &m_uboTest2);

	// TODO: store current swapchain FBO as a global
	FrameBufferVK* currentRenderTarget = m_rendererVK.GetCurrentBackBuffer();

	ContextVK* context = m_rendererVK.GetDevice()->GetCurrentGraphicsContext();
	VkCommandBuffer commandBuffer = context->m_commandBuffer;

	context->BeginPass(currentRenderTarget);

	context->ClearRenderTarget(0, 0, currentRenderTarget->GetWidth(), currentRenderTarget->GetHeight(), { 0.3f, 0.3f, 0.3f, 1.0f }, { 1.0f, 0 });

	// TODO: need to write PSO wrapper. After replace this with context->SetPSO. Orset individual states + final ApplyState call?
	context->SetPipeline(&m_testGraphicsPipeline);

	context->SetVertexBuffer(&m_testVertexBuffer, 0);
	context->SetIndexBuffer(&m_testIndexBuffer, 0);

	// Draw first test cube

	context->SetUniformBuffer(&m_uboBuffers1.GetCurrentBuffer(m_rendererVK.GetDevice()), 0);
	context->SetTexture(&m_testTexture, 0);

	context->CommitBindings(m_rendererVK.GetDevice());

	context->DrawIndexed(m_testIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	// Draw second test cube

	context->SetPipeline(&m_testGraphicsPipeline2);

	context->SetUniformBuffer(&m_uboBuffers2.GetCurrentBuffer(m_rendererVK.GetDevice()), 0);
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
	result &= m_testVertexShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/ApplicationDemo/test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	result &= m_testFragmentShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/ApplicationDemo/test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	result &= m_testFragmentShader2.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/ApplicationDemo/test2.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	assert(result);

	return result;
}

bool ApplicationDemo::CreateUniformBuffers()
{
	m_uboBuffers1.Create(m_rendererVK.GetDevice(), sizeof(UBOTest));
	m_uboBuffers2.Create(m_rendererVK.GetDevice(), sizeof(UBOTest));

	return true;
}

bool ApplicationDemo::CreateGraphicsPipelines()
{
	std::vector<ShaderVK> shaders = { m_testVertexShader, m_testFragmentShader };
	m_testGraphicsPipeline.Create(m_rendererVK.GetDevice(), m_rendererVK.GetCurrentBackBuffer(), shaders);

	std::vector<ShaderVK> shaders2 = { m_testVertexShader, m_testFragmentShader2 };
	m_testGraphicsPipeline2.Create(m_rendererVK.GetDevice(), m_rendererVK.GetCurrentBackBuffer(), shaders2);

	return true;
}

void ApplicationDemo::CreateTestVertexAndTriangleBuffers()
{
	uint32_t size = sizeof(m_testCubeVerts);

	m_testVertexBuffer.Create(m_rendererVK.GetDevice(), size, m_testCubeVerts);

	// index buffer

	size = sizeof(m_testCubeIndices);

	m_testIndexBuffer.Create(m_rendererVK.GetDevice(), size, size / sizeof(uint32_t), false, m_testCubeIndices);
}

void ApplicationDemo::DestroyShaders()
{
	m_testVertexShader.Destroy(m_rendererVK.GetDevice());
	m_testFragmentShader.Destroy(m_rendererVK.GetDevice());

	m_testFragmentShader2.Destroy(m_rendererVK.GetDevice());
}

void ApplicationDemo::DestroyGraphicsPipelines()
{
	m_testGraphicsPipeline.Destroy(m_rendererVK.GetDevice());

	m_testGraphicsPipeline2.Destroy(m_rendererVK.GetDevice());
}

void ApplicationDemo::DestroyTestVertexAndTriangleBuffers()
{
	m_testVertexBuffer.Destroy(m_rendererVK.GetDevice());
	m_testIndexBuffer.Destroy(m_rendererVK.GetDevice());
}

void ApplicationDemo::DestroyUniformBuffers()
{
	m_uboBuffers1.Destroy(m_rendererVK.GetDevice());
	m_uboBuffers2.Destroy(m_rendererVK.GetDevice());
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