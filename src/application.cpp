#include "application.h"

const uint32_t s_windowWidth = 800;
const uint32_t s_windowHeight = 600;

namespace MBRF
{
	void Application::Init()
	{
		m_rendererVK.Init(m_window, s_windowWidth, s_windowHeight);
	}

	void Application::Cleanup()
	{
		m_rendererVK.Cleanup();
	}

	void Application::Update()
	{
		// TODO: add timer to calculate frame timings
		m_rendererVK.Update(1.0f);
	}

	void Application::Draw()
	{
		m_rendererVK.Draw();
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