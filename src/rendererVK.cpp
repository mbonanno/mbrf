#include "rendererVK.h"

#include <iostream>
#include <set>

#include <gtc/matrix_transform.hpp>

namespace MBRF
{

bool RendererVK::Init(GLFWwindow* window, uint32_t width, uint32_t height)
{
	m_currentFrame = 0;

	m_device.Init(&m_swapchain, s_maxFramesInFlight);
	m_swapchain.Init(&m_device);

	// TODO: Init should initialize instance, device, specific textures to our scene

	m_device.CreateInstance(true);

	m_swapchain.CreatePresentationSurface(window);

	m_device.CreateDevice();

	m_swapchain.Create(width, height);

	m_device.CreateSyncObjects();
	m_device.CreateCommandPools();
	m_device.AllocateCommandBuffers();

	m_device.CreateDepthStencilBuffer();
	m_device.CreateTexturesAndSamplers();

	m_device.CreateTestRenderPass();
	m_device.CreateFramebuffers();
	m_device.CreateShaders();

	m_device.CreateDescriptors();

	m_device.CreateGraphicsPipelines();

	m_device.CreateTestVertexAndTriangleBuffers();

	m_device.RecordTestGraphicsCommands();

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
	m_device.DestroyTestRenderPass();

	m_device.DestroyTexturesAndSamplers();
	m_device.DestroyDepthStencilBuffer();

	m_device.DestroyCommandPools();
	m_device.DestroySyncObjects();
	m_swapchain.Cleanup();
	m_device.DestroyDevice();
	m_swapchain.DestroyPresentationSurface();
	m_device.DestroyInstance();
}

void RendererVK::Update(double dt)
{
	// TODO: device shouldn't update anything...fix it
	m_device.Update(dt);
}

void RendererVK::Draw()
{
	// TODO: device shouldn't draw anything...fix it

	m_device.Draw(m_currentFrame);

	m_currentFrame = (m_currentFrame + 1) % s_maxFramesInFlight;
}

}