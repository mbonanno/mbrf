#include "application.h"

const uint32_t s_windowWidth = 800;
const uint32_t s_windowHeight = 600;

namespace MBRF
{
	void Application::Init()
	{
		m_rendererVK.Init(m_window, s_windowWidth, s_windowHeight);

		m_lastFrameTime = std::chrono::steady_clock::now();
	}

	void Application::Cleanup()
	{
		m_rendererVK.Cleanup();
	}

	
	void Application::Update()
	{
		using namespace std::chrono;

		// get frame timings
		auto currentFrameTime = steady_clock::now();
		double dt = duration<double, seconds::period>(currentFrameTime - m_lastFrameTime).count();
		m_lastFrameTime = currentFrameTime;
		
		m_rendererVK.Update(dt);
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