#pragma once

#include "commonVK.h"

#include "glfw/glfw3.h"

namespace MBRF
{

class DeviceVK;

class SwapchainVK
{
public:
	void Init(DeviceVK* deviceRef);

	bool CreatePresentationSurface(GLFWwindow* window);
	void DestroyPresentationSurface();

	bool Create(uint32_t width, uint32_t height);
	void Cleanup();

	uint32_t AcquireNextImage(VkSemaphore semaphore);
	bool PresentQueue(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore);

private:
	bool CreateImageViews();
	void DestroyImageViews();

	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

	DeviceVK* m_deviceRef;

public:
	VkSurfaceKHR m_presentationSurface;
	VkExtent2D m_imageExtent;
	VkFormat m_imageFormat;
	uint32_t m_imageCount;
	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;
};

}
