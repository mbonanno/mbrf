#include "rendererVK.h"

#include <iostream>
#include <set>

#include <gtc/matrix_transform.hpp>

namespace MBRF
{

bool RendererVK::Init(GLFWwindow* window, uint32_t width, uint32_t height)
{
	m_pendingSwapchainResize = false;
	m_swapchainWidth = width;
	m_swapchainHeight = height;

	// Init Vulkan

	m_device.Init(&m_swapchain, window, width, height, s_maxFramesInFlight);

	// Init Scene/Application
	CreateBackBuffer(); // swapchain framebuffer
	CreateTextures();
	CreateTestVertexAndTriangleBuffers();

	// wrap
	CreateShaders();  // Scene specific
	CreateUniformBuffers();  // should be scene specific. Currently it is also submitting the descriptors...
	
	CreateGraphicsPipelines();

	return true;
}

void RendererVK::Cleanup()
{
	m_device.WaitForDevice();

	DestroyGraphicsPipelines();
	DestroyUniformBuffers();
	DestroyShaders();
	
	DestroyTestVertexAndTriangleBuffers();
	DestroyTextures();
	DestroyBackBuffer();

	RenderPassCache::Cleanup(&m_device);
	SamplerCache::Cleanup(&m_device);

	m_device.Cleanup();
}

void RendererVK::RequestSwapchainResize(uint32_t width, uint32_t height)
{
	m_pendingSwapchainResize = true;
	m_swapchainWidth = width;
	m_swapchainHeight = height;
}

void RendererVK::ResizeSwapchain()
{
	m_device.WaitForDevice();

	DestroyBackBuffer();
	DestroyGraphicsPipelines();

	m_device.RecreateSwapchain(m_swapchainWidth, m_swapchainHeight);

	CreateBackBuffer();

	CreateGraphicsPipelines();

	m_pendingSwapchainResize = false;
}

void RendererVK::Update(double dt)
{
	m_uboTest.m_testColor.x += (float)dt;
	if (m_uboTest.m_testColor.x > 1.0f)
		m_uboTest.m_testColor.x = 0.0f;

	m_testCubeRotation += (float)dt;

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), m_testCubeRotation * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), m_backBuffer.m_frameBuffers[0].GetWidth() / float(m_backBuffer.m_frameBuffers[0].GetHeight()), 0.1f, 10.0f);

	// Vulkan clip space has inverted Y and half Z.
	glm::mat4 clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);

	m_uboTest.m_mvpTransform = clip * proj * view * model;

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), m_testCubeRotation * glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	m_uboTest2.m_mvpTransform = clip * proj * view * model;
}

void RendererVK::Draw()
{
	if (!m_device.BeginFrame())
	{
		ResizeSwapchain();
		return;
	}

	uint32_t currentFrameIndex = m_device.m_currentImageIndex;
	VkImage currentSwapchainImage = m_swapchain.m_images[currentFrameIndex];

	// ------------------ Immediate Context Draw -------------------------------
	ContextVK* context = m_device.GetCurrentGraphicsContext();
	VkCommandBuffer commandBuffer = context->m_commandBuffer;

	context->Begin(&m_device);
	m_device.TransitionImageLayout(commandBuffer, currentSwapchainImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	DrawFrame(currentFrameIndex);

	m_device.TransitionImageLayout(commandBuffer, currentSwapchainImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	context->End();
	// ------------------ Immediate Context Draw -------------------------------

	if (!m_device.EndFrame() || m_pendingSwapchainResize)
		ResizeSwapchain();
}

void RendererVK::DrawFrame(uint32_t currentFrameIndex)
{
	// update uniform buffer (TODO: move in ContextVK?)
	m_uboBuffers1.Update(&m_device, currentFrameIndex, &m_uboTest);
	m_uboBuffers2.Update(&m_device, currentFrameIndex, &m_uboTest2);

	// TODO: store current swapchain FBO as a global
	FrameBufferVK* currentRenderTarget = &m_backBuffer.m_frameBuffers[currentFrameIndex];

	ContextVK* context = m_device.GetCurrentGraphicsContext();
	VkCommandBuffer commandBuffer = context->m_commandBuffer;

	context->BeginPass(currentRenderTarget);

	context->ClearRenderTarget(0, 0, currentRenderTarget->GetWidth(), currentRenderTarget->GetHeight(), { 0.3f, 0.3f, 0.3f, 1.0f }, { 1.0f, 0 });

	// TODO: need to write PSO wrapper. After replace this with context->SetPSO. Orset individual states + final ApplyState call?
	context->SetPipeline(&m_testGraphicsPipeline);

	context->SetVertexBuffer(&m_testVertexBuffer, 0);
	context->SetIndexBuffer(&m_testIndexBuffer, 0);

	// Draw first test cube

	context->SetUniformBuffer(&m_uboBuffers1.GetBuffer(currentFrameIndex), 0);
	context->SetTexture(&m_testTexture, 0);

	context->CommitBindings(&m_device);

	context->DrawIndexed(m_testIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	// Draw second test cube

	context->SetPipeline(&m_testGraphicsPipeline2);

	context->SetUniformBuffer(&m_uboBuffers2.GetBuffer(currentFrameIndex), 0);
	context->SetTexture(&m_testTexture2, 0);

	context->CommitBindings(&m_device);

	context->DrawIndexed(m_testIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	context->EndPass();
}

bool RendererVK::CreateBackBuffer()
{
	// Create Depth Stencil

	// TODO: check VK_FORMAT_D24_UNORM_S8_UINT format availability!
	m_backBuffer.m_depthStencilBuffer.Create(&m_device, VK_FORMAT_D24_UNORM_S8_UINT, m_swapchain.m_imageExtent.width, m_swapchain.m_imageExtent.height, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	// Transition (not really needed, we could just set the initial layout to VK_IMAGE_LAYOUT_UNDEFINED in the subpass?)
	m_backBuffer.m_depthStencilBuffer.TransitionImageLayoutAndSubmit(&m_device, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// Create Framebuffers

	m_backBuffer.m_frameBuffers.resize(m_swapchain.m_imageCount);

	for (size_t i = 0; i < m_backBuffer.m_frameBuffers.size(); ++i)
	{
		std::vector<TextureViewVK> attachments = { m_swapchain.m_textureViews[i], m_backBuffer.m_depthStencilBuffer.GetView() };

		m_backBuffer.m_frameBuffers[i].Create(&m_device, m_swapchain.m_imageExtent.width, m_swapchain.m_imageExtent.height, attachments);
	}

	return true;
}

void RendererVK::CreateTextures()
{
	m_testTexture.LoadFromFile(&m_device, "data/textures/test.jpg");
	m_testTexture2.LoadFromFile(&m_device, "data/textures/test2.png");
}

bool RendererVK::CreateShaders()
{
	bool result = true;

	// TODO: put common data/shader dir path in a variable or define
	result &= m_testVertexShader.CreateFromFile(&m_device, "data/shaders/test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	result &= m_testFragmentShader.CreateFromFile(&m_device, "data/shaders/test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	result &= m_testFragmentShader2.CreateFromFile(&m_device, "data/shaders/test2.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	assert(result);

	return result;
}

bool RendererVK::CreateUniformBuffers()
{
	m_uboBuffers1.Create(&m_device, m_swapchain.m_imageCount, sizeof(UBOTest));
	m_uboBuffers2.Create(&m_device, m_swapchain.m_imageCount, sizeof(UBOTest));

	return true;
}

bool RendererVK::CreateGraphicsPipelines()
{
	std::vector<ShaderVK> shaders = { m_testVertexShader, m_testFragmentShader };
	m_testGraphicsPipeline.Create(&m_device, &m_backBuffer.m_frameBuffers[0], shaders);

	std::vector<ShaderVK> shaders2 = { m_testVertexShader, m_testFragmentShader2 };
	m_testGraphicsPipeline2.Create(&m_device, &m_backBuffer.m_frameBuffers[0], shaders2);

	return true;
}

void RendererVK::CreateTestVertexAndTriangleBuffers()
{
	uint32_t size = sizeof(m_testCubeVerts);

	m_testVertexBuffer.Create(&m_device, size, m_testCubeVerts);

	// index buffer

	size = sizeof(m_testCubeIndices);

	m_testIndexBuffer.Create(&m_device, size, size / sizeof(uint32_t), false, m_testCubeIndices);
}

void RendererVK::DestroyShaders()
{
	m_testVertexShader.Destroy(&m_device);
	m_testFragmentShader.Destroy(&m_device);

	m_testFragmentShader2.Destroy(&m_device);
}

void RendererVK::DestroyGraphicsPipelines()
{
	m_testGraphicsPipeline.Destroy(&m_device);

	m_testGraphicsPipeline2.Destroy(&m_device);
}

void RendererVK::DestroyTestVertexAndTriangleBuffers()
{
	m_testVertexBuffer.Destroy(&m_device);
	m_testIndexBuffer.Destroy(&m_device);
}

void RendererVK::DestroyUniformBuffers()
{
	m_uboBuffers1.Destroy(&m_device);
	m_uboBuffers2.Destroy(&m_device);
}

void RendererVK::DestroyBackBuffer()
{
	for (size_t i = 0; i < m_backBuffer.m_frameBuffers.size(); ++i)
		m_backBuffer.m_frameBuffers[i].Destroy(&m_device);

	m_backBuffer.m_depthStencilBuffer.Destroy(&m_device);
}

void RendererVK::DestroyTextures()
{
	m_testTexture.Destroy(&m_device);
	m_testTexture2.Destroy(&m_device);
}

}