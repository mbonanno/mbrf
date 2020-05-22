#pragma once

#include "rendererVK.h"

#include <chrono>

namespace MBRF
{

class Application
{
public:
	void Run();

	void Init();
	void Cleanup();
	void Update();
	void Draw();

	void ResizeWindow();

private:
	RendererVK m_rendererVK;

	GLFWwindow* m_window;

	std::chrono::steady_clock::time_point m_lastFrameTime;
};

}


