#pragma once

#include "rendererVK.h"

#include "bufferVK.h"
#include "frameBufferVK.h"
#include "pipelineVK.h"
#include "shaderVK.h"
#include "textureVK.h"
#include "uniformBufferVK.h"
#include "vertexBufferVK.h"
#include "vertexFormatVK.h"

#include <chrono>

namespace MBRF
{

class Application
{
public:
	virtual ~Application() {};

	void Run();

	void Init();
	void Cleanup();
	void Update();
	void Draw();

	void ResizeWindow();

	void ParseCommandLineArguments(int argc, char **argv);

protected:
	virtual void OnInit() = 0;
	virtual void OnCleanup() = 0;
	virtual void OnResize() = 0;
	virtual void OnUpdate(double dt) = 0;
	virtual void OnDraw() = 0;

protected:
	RendererVK m_rendererVK;
	bool m_enableVulkanValidation = false;

	GLFWwindow* m_window;

	std::chrono::steady_clock::time_point m_lastFrameTime;
};

}


