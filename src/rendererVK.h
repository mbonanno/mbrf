#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // use the Vulkan range of 0.0 to 1.0, instead of the -1 to 1.0 OpenGL range
#include "glm.hpp"

#include "deviceVK.h"
#include "swapchainVK.h"

namespace MBRF
{

class RendererVK
{
public:
	bool Init(GLFWwindow* window, uint32_t width, uint32_t height);
	void Cleanup();

	void Update(double dt);
	void Draw();

private:
	SwapchainVK m_swapchain;
	DeviceVK m_device;

	static const int s_maxFramesInFlight = 2;
};

}