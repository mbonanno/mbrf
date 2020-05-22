#include "application.h"

const uint32_t s_windowWidth = 800;
const uint32_t s_windowHeight = 600;

namespace MBRF
{

void Application::Init()
{
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);

	m_rendererVK.Init(m_window, width, height);

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

void Application::ResizeWindow()
{
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);

	// handle minimized window
	while (width == 0 || height == 0) 
	{
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	m_rendererVK.RequestSwapchainResize(width, height);
}

static void resizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	app->ResizeWindow();
}

void Application::Run()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(s_windowWidth, s_windowHeight, "MBRF", nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, resizeCallback);
		
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