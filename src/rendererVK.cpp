#include "rendererVK.h"

#include "frameBufferVK.h"

namespace MBRF
{

bool RendererVK::Init(GLFWwindow* window, uint32_t width, uint32_t height, bool enableValidation)
{
	m_pendingSwapchainResize = false;
	m_swapchainWidth = width;
	m_swapchainHeight = height;

	// Init Vulkan

	m_device.Init(&m_swapchain, window, width, height, s_maxFramesInFlight, enableValidation);

	CreateBackBuffer(); // swapchain framebuffer

	return true;
}

void RendererVK::Cleanup()
{
	DestroyBackBuffer();

	RenderPassCache::Cleanup(&m_device);
	SamplerCache::Cleanup(&m_device);

	m_device.Cleanup();
}

void RendererVK::WaitForDevice()
{
	m_device.WaitForDevice();
}

void RendererVK::RequestSwapchainResize(uint32_t width, uint32_t height, std::function<void()> const &onResizeCallback)
{
	m_pendingSwapchainResize = true;
	m_swapchainWidth = width;
	m_swapchainHeight = height;

	m_resizeCallback = onResizeCallback;
}

void RendererVK::ResizeSwapchain()
{
	m_device.WaitForDevice();

	DestroyBackBuffer();

	m_device.RecreateSwapchain(m_swapchainWidth, m_swapchainHeight);

	CreateBackBuffer();

	m_resizeCallback();

	m_pendingSwapchainResize = false;
}

bool RendererVK::BeginDraw()
{
	if (!m_device.BeginFrame())
	{
		ResizeSwapchain();
		return false;
	}

	uint32_t currentFrameIndex = m_device.m_currentImageIndex;
	VkImage currentSwapchainImage = m_swapchain.m_images[currentFrameIndex];

	// ------------------ Immediate Context Draw -------------------------------
	ContextVK* context = m_device.GetCurrentGraphicsContext();
	VkCommandBuffer commandBuffer = context->m_commandBuffer;

	context->Begin(&m_device);
	m_device.TransitionImageLayout(commandBuffer, currentSwapchainImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	return true;
}

void RendererVK::EndDraw()
{
	uint32_t currentFrameIndex = m_device.m_currentImageIndex;
	VkImage currentSwapchainImage = m_swapchain.m_images[currentFrameIndex];
	ContextVK* context = m_device.GetCurrentGraphicsContext();
	VkCommandBuffer commandBuffer = context->m_commandBuffer;

	m_device.TransitionImageLayout(commandBuffer, currentSwapchainImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	context->End();
	// ------------------ Immediate Context Draw -------------------------------

	if (!m_device.EndFrame() || m_pendingSwapchainResize)
		ResizeSwapchain();
}

bool RendererVK::CreateBackBuffer()
{
	// Create Depth Stencil

	// TODO: check VK_FORMAT_D24_UNORM_S8_UINT format availability!
	m_backBuffer.m_depthStencilBuffer.Create(&m_device, VK_FORMAT_D24_UNORM_S8_UINT, m_swapchain.m_imageExtent.width, m_swapchain.m_imageExtent.height, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	// Transition (not really needed, we could just set the initial layout to VK_IMAGE_LAYOUT_UNDEFINED in the subpass?)
	m_backBuffer.m_depthStencilBuffer.TransitionImageLayoutAndSubmit(&m_device, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// Create Framebuffers

	m_backBuffer.m_frameBuffers.resize(m_swapchain.m_imageCount);

	for (size_t i = 0; i < m_backBuffer.m_frameBuffers.size(); ++i)
	{
		std::vector<TextureViewVK> attachments = { m_swapchain.m_textureViews[i], m_backBuffer.m_depthStencilBuffer.GetView() };

		m_backBuffer.m_frameBuffers[i].Create(&m_device, m_swapchain.m_imageExtent.width, m_swapchain.m_imageExtent.height, attachments);
	}

	return true;
}

FrameBufferVK* RendererVK::GetCurrentBackBuffer()
{
	return &m_backBuffer.m_frameBuffers[m_device.m_currentImageIndex];
}

void RendererVK::DestroyBackBuffer()
{
	for (size_t i = 0; i < m_backBuffer.m_frameBuffers.size(); ++i)
		m_backBuffer.m_frameBuffers[i].Destroy(&m_device);

	m_backBuffer.m_depthStencilBuffer.Destroy(&m_device);
}

}