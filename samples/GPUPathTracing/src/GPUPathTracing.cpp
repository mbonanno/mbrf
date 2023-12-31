#include "GPUPathTracing.h"

namespace MBRF
{

void GPUPathTracing::OnInit()
{
	CreateRenderTargets();
	CreateTestVertexAndTriangleBuffers();

	CreateShaders();

	CreateGraphicsPipelines();

	m_cubemap.LoadFromKTXFile(m_rendererVK.GetDevice(), "../../data/textures/cubemap_yokohama_bc3_unorm.ktx", VK_FORMAT_BC3_SRGB_BLOCK);
}

void GPUPathTracing::OnCleanup()
{
	m_cubemap.Destroy(m_rendererVK.GetDevice());

	DestroyGraphicsPipelines();
	DestroyShaders();

	DestroyTestVertexAndTriangleBuffers();
	DestroyRenderTargets();
}

void GPUPathTracing::OnResize()
{
	DestroyRenderTargets();
	CreateRenderTargets();

	DestroyGraphicsPipelines();
	CreateGraphicsPipelines();

	m_numFrames = 0;
}

void GPUPathTracing::OnUpdate(double dt)
{

}

// 2 passes:
// - Path Tracing compute pass
// - render the result to a full screen quad

void GPUPathTracing::OnDraw()
{
	// TODO: implement transitions batching!

	ContextVK* context = m_rendererVK.GetDevice()->GetCurrentGraphicsContext();

	// Compute Pass: Path Tracing

	context->TransitionImageLayout(m_rendererVK.GetDevice(), &m_computeTarget, VK_IMAGE_LAYOUT_GENERAL);

	context->SetPipeline(&m_computePipeline);

	struct ComputeConsts
	{
		glm::vec2 resolution;
		int numFrame;
	} compConsts;

	compConsts.numFrame = m_numFrames;
	compConsts.resolution = glm::vec2(m_computeTarget.GetWidth(), m_computeTarget.GetHeight());

	context->SetUniformBuffer(m_rendererVK.GetDevice(), &compConsts, sizeof(ComputeConsts), 0);
	context->SetStorageImage(&m_computeTarget, 0);
	context->SetTexture(&m_cubemap, 0);

	context->CommitBindings(m_rendererVK.GetDevice());

	uint32_t threadGroupSize = 16;
	uint32_t dispatchSizes[3] = { m_computeTarget.GetWidth() / threadGroupSize, m_computeTarget.GetHeight() / threadGroupSize, 1 };
	context->Dispatch(dispatchSizes[0], dispatchSizes[1], dispatchSizes[2]);

	// Draw fullscreen quad

	context->TransitionImageLayout(m_rendererVK.GetDevice(), &m_computeTarget, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	FrameBufferVK* currentRenderTarget = m_rendererVK.GetCurrentBackBuffer();

	context->BeginPass(currentRenderTarget);

	context->ClearRenderTarget(0, 0, currentRenderTarget->GetWidth(), currentRenderTarget->GetHeight(), { 0.3f, 0.3f, 0.3f, 1.0f }, { 1.0f, 0 });

	context->SetPipeline(&m_postProcPipeline);

	context->SetVertexBuffer(&m_quadVertexBuffer, 0);
	context->SetIndexBuffer(&m_quadIndexBuffer, 0);

	context->SetTexture(&m_computeTarget, 0);

	context->CommitBindings(m_rendererVK.GetDevice());

	context->DrawIndexed(m_quadIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	context->EndPass();

	m_numFrames++;
}

void GPUPathTracing::CreateRenderTargets()
{
	uint32_t width = m_rendererVK.GetCurrentBackBuffer()->GetWidth();
	uint32_t height = m_rendererVK.GetCurrentBackBuffer()->GetHeight();

	m_computeTarget.Create(m_rendererVK.GetDevice(), VK_FORMAT_R32G32B32A32_SFLOAT, width, height, 1, 1, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	m_computeTarget.TransitionImageLayoutAndSubmit(m_rendererVK.GetDevice(), VK_IMAGE_LAYOUT_GENERAL);
}

bool GPUPathTracing::CreateShaders()
{
	bool result = true;

	// TODO: put common data/shader dir path in a variable or define
	result &= m_quadVertexShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/GPUPathTracing/quad.vert.spv", SHADER_STAGE_VERTEX);
	result &= m_quadFragmentShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/GPUPathTracing/quad.frag.spv", SHADER_STAGE_FRAGMENT);

	result &= m_computeShader.CreateFromFile(m_rendererVK.GetDevice(), "../../data/shaders/GPUPathTracing/pathTracing.comp.spv", SHADER_STAGE_COMPUTE);

	assert(result);

	return result;
}

bool GPUPathTracing::CreateGraphicsPipelines()
{
	// QUAD

	GraphicsPipelineDesc desc;

	desc.m_vertexFormat = &m_vertexFormatPosUV;
	desc.m_frameBuffer = m_rendererVK.GetCurrentBackBuffer();
	desc.m_shaders = { m_quadVertexShader, m_quadFragmentShader };
	desc.m_cullMode = CULL_MODE_NONE;

	m_postProcPipeline.Create(m_rendererVK.GetDevice(), desc);

	// COMPUTE

	m_computePipeline.Create(m_rendererVK.GetDevice(), &m_computeShader);

	return true;
}

void GPUPathTracing::CreateTestVertexAndTriangleBuffers()
{
	// QUAD

	uint32_t size = sizeof(m_quadVerts);

	m_quadVertexBuffer.Create(m_rendererVK.GetDevice(), size, m_quadVerts);

	// index buffer

	size = sizeof(m_quadIndices);

	m_quadIndexBuffer.Create(m_rendererVK.GetDevice(), size, size / sizeof(uint32_t), false, m_quadIndices);

	// Vertex format

	m_vertexFormatPosUV.AddAttribute(VertexAttributeVK(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexPosUV, m_pos), sizeof(VertexPosUV::m_pos)));
	m_vertexFormatPosUV.AddAttribute(VertexAttributeVK(1, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexPosUV, m_texcoord), sizeof(VertexPosUV::m_texcoord)));
}

void GPUPathTracing::DestroyShaders()
{
	m_quadVertexShader.Destroy(m_rendererVK.GetDevice());
	m_quadFragmentShader.Destroy(m_rendererVK.GetDevice());

	m_computeShader.Destroy(m_rendererVK.GetDevice());
}

void GPUPathTracing::DestroyGraphicsPipelines()
{
	m_postProcPipeline.Destroy(m_rendererVK.GetDevice());

	m_computePipeline.Destroy(m_rendererVK.GetDevice());
}

void GPUPathTracing::DestroyTestVertexAndTriangleBuffers()
{
	m_quadVertexBuffer.Destroy(m_rendererVK.GetDevice());
	m_quadIndexBuffer.Destroy(m_rendererVK.GetDevice());
}

void GPUPathTracing::DestroyRenderTargets()
{
	m_computeTarget.Destroy(m_rendererVK.GetDevice());
}

}

int main(int argc, char **argv)
{
	MBRF::GPUPathTracing app;

	app.ParseCommandLineArguments(argc, argv);

	app.Run();

	return 0;
}