#include "postProcessing.h"

namespace MBRF
{

void PostProcessing::OnInit()
{
	CreateTextures();
	CreateTestVertexAndTriangleBuffers();

	CreateShaders();

	CreateGraphicsPipelines();
}

void PostProcessing::OnCleanup()
{
	DestroyGraphicsPipelines();
	DestroyShaders();

	DestroyTestVertexAndTriangleBuffers();
	DestroyTextures();
}

void PostProcessing::OnResize()
{
	DestroyRenderTargets();
	CreateRenderTargets();


	DestroyGraphicsPipelines();
	CreateGraphicsPipelines();
}

void PostProcessing::OnUpdate(double dt)
{
	m_cubeRotation += (float)dt;

	FrameBufferVK* backBuffer = m_rendererVK.GetCurrentBackBuffer();

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), m_cubeRotation * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), backBuffer->GetWidth() / float(backBuffer->GetHeight()), m_nearPlane, m_farPlane);

	// Vulkan clip space has inverted Y and half Z.
	glm::mat4 clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);

	m_sceneUniforms.m_mvpTransform = clip * proj * view * model;
}

// 3 passes:
// - render the scene offscreen
// - apply any needed postprocessing in the compute pass (2 pass separable gaussian blur)
// - render the result to a full screen quad

void PostProcessing::OnDraw()
{
	// TODO: implement transitions batching!

	ContextVK* context = m_rendererVK.GetDevice()->GetCurrentGraphicsContext();

	// Draw scene offscreen

	context->TransitionImageLayout(m_rendererVK.GetDevice(), &m_renderTarget, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	context->TransitionImageLayout(m_rendererVK.GetDevice(), &m_offscreenDepthStencil, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	FrameBufferVK* currentRenderTarget = &m_offscreenFramebuffer;

	context->BeginPass(currentRenderTarget);
	context->ClearRenderTarget(0, 0, currentRenderTarget->GetWidth(), currentRenderTarget->GetHeight(), { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0 });

	context->SetPipeline(&m_offscreenPipeline);

	context->SetVertexBuffer(&m_cubeVertexBuffer, 0);
	context->SetIndexBuffer(&m_cubeIndexBuffer, 0);
	context->SetUniformBuffer(m_rendererVK.GetDevice(), &m_sceneUniforms, sizeof(SceneUniforms), 0);
	context->SetTexture(&m_sceneTexture, 0);

	context->CommitBindings(m_rendererVK.GetDevice());

	context->DrawIndexed(m_cubeIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	context->EndPass();

	// Compute Pass

	context->TransitionImageLayout(m_rendererVK.GetDevice(), &m_renderTarget, VK_IMAGE_LAYOUT_GENERAL);
	context->TransitionImageLayout(m_rendererVK.GetDevice(), &m_computeTarget, VK_IMAGE_LAYOUT_GENERAL);

	context->SetPipeline(&m_computePipeline);

	// Horizontal

	struct ComputeConsts
	{
		uint32_t horizontal;
	} compConsts;

	compConsts.horizontal = 1;


	context->SetUniformBuffer(m_rendererVK.GetDevice(), &compConsts, sizeof(ComputeConsts), 0);
	context->SetStorageImage(&m_renderTarget, 0);
	context->SetStorageImage(&m_computeTarget, 1);

	context->CommitBindings(m_rendererVK.GetDevice());

	uint32_t threadGroupSize = 16;
	uint32_t dispatchSizes[3] = { m_computeTarget.GetWidth() / threadGroupSize, m_computeTarget.GetHeight() / threadGroupSize, 1 };
	context->Dispatch(dispatchSizes[0], dispatchSizes[1], dispatchSizes[2]);

	// Vertical

	compConsts.horizontal = 0;

	context->SetUniformBuffer(m_rendererVK.GetDevice(), &compConsts, sizeof(ComputeConsts), 0);
	context->SetStorageImage(&m_computeTarget, 0);
	context->SetStorageImage(&m_renderTarget, 1);

	context->CommitBindings(m_rendererVK.GetDevice());

	context->Dispatch(dispatchSizes[0], dispatchSizes[1], dispatchSizes[2]);

	// Draw fullscreen quad

	context->TransitionImageLayout(m_rendererVK.GetDevice(), &m_renderTarget, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	context->TransitionImageLayout(m_rendererVK.GetDevice(), &m_offscreenDepthStencil, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	currentRenderTarget = m_rendererVK.GetCurrentBackBuffer();

	context->BeginPass(currentRenderTarget);

	context->ClearRenderTarget(0, 0, currentRenderTarget->GetWidth(), currentRenderTarget->GetHeight(), { 0.3f, 0.3f, 0.3f, 1.0f }, { 1.0f, 0 });

	context->SetPipeline(&m_postProcPipeline);

	context->SetVertexBuffer(&m_quadVertexBuffer, 0);
	context->SetIndexBuffer(&m_quadIndexBuffer, 0);

	struct PostProcConsts
	{
		float nearPlane;
		float farPlane;
	}postProcConsts;

	postProcConsts.nearPlane = m_nearPlane;
	postProcConsts.farPlane = m_farPlane;

	context->SetUniformBuffer(m_rendererVK.GetDevice(), &postProcConsts, sizeof(PostProcConsts), 0);
	context->SetTexture(&m_renderTarget, 0);
	context->SetTexture(&m_offscreenDepthStencil, 1);
	context->SetTexture(&m_vignetteTexture, 2);

	context->CommitBindings(m_rendererVK.GetDevice());

	context->DrawIndexed(m_quadIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	context->EndPass();
}

void PostProcessing::CreateRenderTargets()
{
	uint32_t width = m_rendererVK.GetCurrentBackBuffer()->GetWidth();
	uint32_t height = m_rendererVK.GetCurrentBackBuffer()->GetHeight();

	m_renderTarget.Create(m_rendererVK.GetDevice(), VK_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_renderTarget.TransitionImageLayoutAndSubmit(m_rendererVK.GetDevice(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	m_offscreenDepthStencil.Create(m_rendererVK.GetDevice(), VK_FORMAT_D16_UNORM, width, height, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	m_offscreenDepthStencil.TransitionImageLayoutAndSubmit(m_rendererVK.GetDevice(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);


	m_computeTarget.Create(m_rendererVK.GetDevice(), VK_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	m_computeTarget.TransitionImageLayoutAndSubmit(m_rendererVK.GetDevice(), VK_IMAGE_LAYOUT_GENERAL);

	std::vector<TextureViewVK> attachments = { m_renderTarget.GetView(), m_offscreenDepthStencil.GetView() };
	m_offscreenFramebuffer.Create(m_rendererVK.GetDevice(), width, height, attachments);
}

void PostProcessing::CreateTextures()
{
	CreateRenderTargets();

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

	result &= m_computeShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/PostProcessing/blur.comp.spv", SHADER_STAGE_COMPUTE);

	assert(result);

	return result;
}

bool PostProcessing::CreateGraphicsPipelines()
{
	// OFFSCREEN

	GraphicsPipelineDesc desc;
	desc.m_vertexFormat = &m_vertexFormatPosUV;
	desc.m_frameBuffer = &m_offscreenFramebuffer;
	desc.m_shaders = { m_offscreenVertexShader, m_offscreenFragmentShader };
	desc.m_cullMode = CULL_MODE_NONE;

	m_offscreenPipeline.Create(m_rendererVK.GetDevice(), desc);

	// QUAD

	desc.m_frameBuffer = m_rendererVK.GetCurrentBackBuffer();
	desc.m_shaders = { m_quadVertexShader, m_quadFragmentShader };
	desc.m_cullMode = CULL_MODE_NONE;

	m_postProcPipeline.Create(m_rendererVK.GetDevice(), desc);

	// COMPUTE

	m_computePipeline.Create(m_rendererVK.GetDevice(), &m_computeShader);

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

	m_computeShader.Destroy(m_rendererVK.GetDevice());
}

void PostProcessing::DestroyGraphicsPipelines()
{
	m_offscreenPipeline.Destroy(m_rendererVK.GetDevice());

	m_postProcPipeline.Destroy(m_rendererVK.GetDevice());

	m_computePipeline.Destroy(m_rendererVK.GetDevice());
}

void PostProcessing::DestroyTestVertexAndTriangleBuffers()
{
	m_cubeVertexBuffer.Destroy(m_rendererVK.GetDevice());
	m_cubeIndexBuffer.Destroy(m_rendererVK.GetDevice());

	m_quadVertexBuffer.Destroy(m_rendererVK.GetDevice());
	m_quadIndexBuffer.Destroy(m_rendererVK.GetDevice());
}

void PostProcessing::DestroyRenderTargets()
{
	m_offscreenFramebuffer.Destroy(m_rendererVK.GetDevice());

	m_renderTarget.Destroy(m_rendererVK.GetDevice());
	m_offscreenDepthStencil.Destroy(m_rendererVK.GetDevice());

	m_computeTarget.Destroy(m_rendererVK.GetDevice());
}

void PostProcessing::DestroyTextures()
{
	DestroyRenderTargets();

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