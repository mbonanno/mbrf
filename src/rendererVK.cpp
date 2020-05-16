#include "rendererVK.h"

#include <iostream>
#include <set>

#include <gtc/matrix_transform.hpp>

namespace MBRF
{

bool RendererVK::Init(GLFWwindow* window, uint32_t width, uint32_t height)
{
	// Init Vulkan

	m_device.Init(&m_swapchain, window, width, height, s_maxFramesInFlight);

	// TODO: Init should initialize instance, device, specific textures to our scene

	// Init Scene/Application
	m_device.CreateDepthStencilBuffer();  
	m_device.CreateFramebuffers(); // swapchain framebuffer

	m_device.CreateTextures();  // Scene specific
	m_device.CreateShaders();  // Scene specific
	m_device.CreateDescriptors();  // should be scene specific. Currently it is also submitting the descriptors...
	m_device.CreateGraphicsPipelines();  // Scene specific
	m_device.CreateTestVertexAndTriangleBuffers();  // Scene specific

	m_device.RecordTestGraphicsCommands();  // Scene specific/should be handled by the GraphicsContext. Shouldn't be pre-recorded, but dynamic per frame

	return true;
}

void RendererVK::Cleanup()
{
	// TODO: this should release all the resources used + instances etc

	m_device.WaitForDevice();

	m_device.DestroyTestVertexAndTriangleBuffers();

	m_device.DestroyDescriptors();

	m_device.DestroyGraphicsPipelines();
	m_device.DestroyShaders();
	m_device.DestroyFramebuffers();

	m_device.DestroyTextures();
	m_device.DestroyDepthStencilBuffer();

	RenderPassCache::Cleanup(&m_device);
	SamplerCache::Cleanup(&m_device);

	m_device.Cleanup();
}

void RendererVK::Update(double dt)
{
	// TODO: device shouldn't update anything...fix it
	m_device.Update(dt);
}

void RendererVK::Draw()
{
	m_device.BeginFrame();

	// TODO: device shouldn't draw anything...fix it
	m_device.Draw();

	m_device.EndFrame();
}

}