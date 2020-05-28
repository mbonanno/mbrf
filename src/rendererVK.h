#pragma once

#include "deviceVK.h"
#include "swapchainVK.h"

#include <functional>

namespace MBRF
{

class RendererVK
{
public:
	bool Init(GLFWwindow* window, uint32_t width, uint32_t height, bool enableValidation);
	void Cleanup();

	void WaitForDevice();

	bool BeginDraw();
	void EndDraw();

	void RequestSwapchainResize(uint32_t width, uint32_t height, std::function<void()> const &onResizeCallback);

	DeviceVK* GetDevice() { return &m_device; };

	FrameBufferVK* GetCurrentBackBuffer();

private:
	void ResizeSwapchain();

	bool CreateBackBuffer();
	void DestroyBackBuffer();

	std::function<void()> m_resizeCallback;

private:
	SwapchainVK m_swapchain;
	DeviceVK m_device;

	bool m_pendingSwapchainResize = false;
	uint32_t m_swapchainWidth = 0;
	uint32_t m_swapchainHeight = 0;

	static const int s_maxFramesInFlight = 2;

	struct BackBuffer
	{
		std::vector<FrameBufferVK> m_frameBuffers;
		TextureVK m_depthStencilBuffer;
	};

	BackBuffer m_backBuffer;
};

}
