#include "postProcessing.h"

namespace MBRF
{

void PostProcessing::OnInit()
{
	CreateTextures();
	CreateTestVertexAndTriangleBuffers();

	CreateShaders();
	CreateUniformBuffers();

	CreateGraphicsPipelines();
}

void PostProcessing::OnCleanup()
{
	DestroyGraphicsPipelines();
	DestroyUniformBuffers();
	DestroyShaders();

	DestroyTestVertexAndTriangleBuffers();
	DestroyTextures();
}

void PostProcessing::OnResize()
{
	DestroyGraphicsPipelines();
	CreateGraphicsPipelines();
}

void PostProcessing::OnUpdate(double dt)
{
	m_uboTest.m_testColor.x += (float)dt;
	if (m_uboTest.m_testColor.x > 1.0f)
		m_uboTest.m_testColor.x = 0.0f;

	m_cubeRotation += (float)dt;

	FrameBufferVK* backBuffer = m_rendererVK.GetCurrentBackBuffer();

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), m_cubeRotation * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), backBuffer->GetWidth() / float(backBuffer->GetHeight()), 0.1f, 10.0f);

	// Vulkan clip space has inverted Y and half Z.
	glm::mat4 clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);

	m_uboTest.m_mvpTransform = clip * proj * view * model;
}

void PostProcessing::OnDraw()
{
	m_uniformBuffer.UpdateCurrentBuffer(m_rendererVK.GetDevice(), &m_uboTest);

	ContextVK* context = m_rendererVK.GetDevice()->GetCurrentGraphicsContext();
	VkCommandBuffer commandBuffer = context->m_commandBuffer;

	// Draw scene offscreen

	m_renderTarget.TransitionImageLayout(m_rendererVK.GetDevice(), commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	FrameBufferVK* currentRenderTarget = &m_offscreenFramebuffer;

	context->BeginPass(currentRenderTarget);
	context->ClearRenderTarget(0, 0, currentRenderTarget->GetWidth(), currentRenderTarget->GetHeight(), { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0 });

	context->SetPipeline(&m_offscreenPipeline);

	context->SetVertexBuffer(&m_cubeVertexBuffer, 0);
	context->SetIndexBuffer(&m_cubeIndexBuffer, 0);
	context->SetUniformBuffer(&m_uniformBuffer.GetCurrentBuffer(m_rendererVK.GetDevice()), 0);
	context->SetTexture(&m_sceneTexture, 0);

	context->CommitBindings(m_rendererVK.GetDevice());

	context->DrawIndexed(m_cubeIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	context->EndPass();

	m_renderTarget.TransitionImageLayout(m_rendererVK.GetDevice(), commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Draw fullscreen quad

	currentRenderTarget = m_rendererVK.GetCurrentBackBuffer();

	context->BeginPass(currentRenderTarget);

	context->ClearRenderTarget(0, 0, currentRenderTarget->GetWidth(), currentRenderTarget->GetHeight(), { 0.3f, 0.3f, 0.3f, 1.0f }, { 1.0f, 0 });

	context->SetPipeline(&m_postProcPipeline);

	context->SetVertexBuffer(&m_quadVertexBuffer, 0);
	context->SetIndexBuffer(&m_quadIndexBuffer, 0);

	context->SetTexture(&m_renderTarget, 0);
	context->SetTexture(&m_vignetteTexture, 1);

	context->CommitBindings(m_rendererVK.GetDevice());

	context->DrawIndexed(m_quadIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	context->EndPass();
}

void PostProcessing::CreateTextures()
{
	uint32_t width = m_rendererVK.GetCurrentBackBuffer()->GetWidth();
	uint32_t height = m_rendererVK.GetCurrentBackBuffer()->GetHeight();

	m_renderTarget.Create(m_rendererVK.GetDevice(), VK_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	m_renderTarget.TransitionImageLayoutAndSubmit(m_rendererVK.GetDevice(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_offscreenDepthStencil.Create(m_rendererVK.GetDevice(), VK_FORMAT_D24_UNORM_S8_UINT, width, height, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	m_offscreenDepthStencil.TransitionImageLayoutAndSubmit(m_rendererVK.GetDevice(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


	std::vector<TextureViewVK> attachments = { m_renderTarget.GetView(), m_offscreenDepthStencil.GetView() };
	m_offscreenFramebuffer.Create(m_rendererVK.GetDevice(), width, height, attachments);

	m_sceneTexture.LoadFromFile(m_rendererVK.GetDevice(), "../../data/textures/test.jpg");
	m_vignetteTexture.LoadFromFile(m_rendererVK.GetDevice(), "../../data/textures/vignette.jpg");
}

bool PostProcessing::CreateShaders()
{
	bool result = true;

	// TODO: put common data/shader dir path in a variable or define
	result &= m_offscreenVertexShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/PostProcessing/scene.vert.spv", SHADER_STAGE_VERTEX);
	result &= m_offscreenFragmentShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/PostProcessing/scene.frag.spv", SHADER_STAGE_FRAGMENT);

	result &= m_quadVertexShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/PostProcessing/quad.vert.spv", SHADER_STAGE_VERTEX);
	result &= m_quadFragmentShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/PostProcessing/quad.frag.spv", SHADER_STAGE_FRAGMENT);

	assert(result);

	return result;
}

bool PostProcessing::CreateUniformBuffers()
{
	m_uniformBuffer.Create(m_rendererVK.GetDevice(), sizeof(UBOTest));

	return true;
}

bool PostProcessing::CreateGraphicsPipelines()
{
	// OFFSCREEN

	GraphicsPipelineDesc desc;
	desc.m_vertexFormat = &m_vertexFormatPosUV;
	desc.m_frameBuffer = &m_offscreenFramebuffer;
	desc.m_shaders = { m_offscreenVertexShader, m_offscreenFragmentShader };
	desc.m_cullMode = CULL_MODE_BACK;

	m_offscreenPipeline.Create(m_rendererVK.GetDevice(), desc);

	// QUAD

	desc.m_frameBuffer = m_rendererVK.GetCurrentBackBuffer();
	desc.m_shaders = { m_quadVertexShader, m_quadFragmentShader };
	desc.m_cullMode = CULL_MODE_NONE;

	m_postProcPipeline.Create(m_rendererVK.GetDevice(), desc);

	return true;
}

void PostProcessing::CreateTestVertexAndTriangleBuffers()
{
	// vertex buffer

	uint32_t size = sizeof(m_testCubeVerts);

	m_cubeVertexBuffer.Create(m_rendererVK.GetDevice(), size, m_testCubeVerts);

	// index buffer

	size = sizeof(m_testCubeIndices);

	m_cubeIndexBuffer.Create(m_rendererVK.GetDevice(), size, size / sizeof(uint32_t), false, m_testCubeIndices);


	// QUAD

	size = sizeof(m_quadVerts);

	m_quadVertexBuffer.Create(m_rendererVK.GetDevice(), size, m_quadVerts);

	// index buffer

	size = sizeof(m_quadIndices);

	m_quadIndexBuffer.Create(m_rendererVK.GetDevice(), size, size / sizeof(uint32_t), false, m_quadIndices);

	// Vertex format

	m_vertexFormatPosUV.AddAttribute(VertexAttributeVK(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPosUV, m_pos), sizeof(VertexPosUV::m_pos)));
	m_vertexFormatPosUV.AddAttribute(VertexAttributeVK(1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexPosUV, m_texcoord), sizeof(VertexPosUV::m_texcoord)));
}

void PostProcessing::DestroyShaders()
{
	m_offscreenVertexShader.Destroy(m_rendererVK.GetDevice());
	m_offscreenFragmentShader.Destroy(m_rendererVK.GetDevice());

	m_quadVertexShader.Destroy(m_rendererVK.GetDevice());
	m_quadFragmentShader.Destroy(m_rendererVK.GetDevice());
}

void PostProcessing::DestroyGraphicsPipelines()
{
	m_offscreenPipeline.Destroy(m_rendererVK.GetDevice());

	m_postProcPipeline.Destroy(m_rendererVK.GetDevice());
}

void PostProcessing::DestroyTestVertexAndTriangleBuffers()
{
	m_cubeVertexBuffer.Destroy(m_rendererVK.GetDevice());
	m_cubeIndexBuffer.Destroy(m_rendererVK.GetDevice());

	m_quadVertexBuffer.Destroy(m_rendererVK.GetDevice());
	m_quadIndexBuffer.Destroy(m_rendererVK.GetDevice());
}

void PostProcessing::DestroyUniformBuffers()
{
	m_uniformBuffer.Destroy(m_rendererVK.GetDevice());
}

void PostProcessing::DestroyTextures()
{
	m_offscreenFramebuffer.Destroy(m_rendererVK.GetDevice());

	m_renderTarget.Destroy(m_rendererVK.GetDevice());
	m_offscreenDepthStencil.Destroy(m_rendererVK.GetDevice());

	m_sceneTexture.Destroy(m_rendererVK.GetDevice());
	m_vignetteTexture.Destroy(m_rendererVK.GetDevice());
}

}

int main(int argc, char **argv)
{
	MBRF::PostProcessing app;

	app.ParseCommandLineArguments(argc, argv);

	app.Run();

	return 0;
}