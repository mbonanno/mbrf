#include "application.h"

const uint32_t s_windowWidth = 800;
const uint32_t s_windowHeight = 600;

namespace MBRF
{
	void Application::Init()
	{
		currentFrame = 0;

		m_rendererVK.CreateInstance(true);
		m_rendererVK.CreatePresentationSurface(m_window);
		m_rendererVK.CreateDevice();
		m_rendererVK.CreateSwapchain(s_windowWidth, s_windowHeight);
		m_rendererVK.CreateSwapchainImageViews();
		m_rendererVK.CreateSyncObjects(s_MaxFramesInFlight);
		m_rendererVK.CreateCommandPools();
		m_rendererVK.AllocateCommandBuffers();
		m_rendererVK.CreateTestRenderPass();
		m_rendererVK.CreateFramebuffers();
		m_rendererVK.CreateShaders();
		m_rendererVK.CreateGraphicsPipelines();

		m_rendererVK.RecordTestGraphicsCommands();
	}

	void Application::Cleanup()
	{
		m_rendererVK.WaitForDevice();

		m_rendererVK.DestroyGraphicsPipelines();
		m_rendererVK.DestroyShaders();
		m_rendererVK.DestroyFramebuffers();
		m_rendererVK.DestroyTestRenderPass();
		m_rendererVK.DestroyCommandPools();
		m_rendererVK.DestroySyncObjects();
		m_rendererVK.DestroySwapchainImageViews();
		m_rendererVK.DestroySwapchain();
		m_rendererVK.DestroyDevice();
		m_rendererVK.DestroyPresentationSurface();
		m_rendererVK.DestroyInstance();
	}

	void Application::Update()
	{

	}

	void Application::Draw()
	{
		uint32_t imageIndex = m_rendererVK.AcquireNextSwapchainImage(currentFrame);

		assert(imageIndex != UINT32_MAX);

		m_rendererVK.SubmitGraphicsQueue(imageIndex, currentFrame);

		m_rendererVK.PresentSwapchainImage(imageIndex, currentFrame);

		currentFrame = (currentFrame + 1) % s_MaxFramesInFlight;
	}

	void Application::Run()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_window = glfwCreateWindow(s_windowWidth, s_windowHeight, "MBRF", nullptr, nullptr);
		
		Init();

		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();

			Update();
			Draw();
		}

		Cleanup();

		glfwDestroyWindow(m_window);
		glfwTerminate();
	}
	
}