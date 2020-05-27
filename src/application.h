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

	void ParseCommandLineArguments(int argc, char **argv);

private:
	RendererVK m_rendererVK;
	bool m_enableVulkanValidation = false;

	GLFWwindow* m_window;

	std::chrono::steady_clock::time_point m_lastFrameTime;
};

}


