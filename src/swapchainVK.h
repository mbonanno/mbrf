#pragma once

#include "commonVK.h"

#include "glfw/glfw3.h"

namespace MBRF
{

class DeviceVK;

class SwapchainVK
{
public:
	bool CreatePresentationSurface(DeviceVK* device, GLFWwindow* window);
	void DestroyPresentationSurface(DeviceVK* device);

	bool Create(DeviceVK* device, uint32_t width, uint32_t height);
	void Destroy(DeviceVK* device);

	uint32_t AcquireNextImage(DeviceVK* device, VkSemaphore semaphore);

private:
	bool CreateImageViews(DeviceVK* device);
	void DestroyImageViews(DeviceVK* device);

public:
	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;

	VkSurfaceKHR m_presentationSurface;
	VkExtent2D m_imageExtent;
	VkFormat m_imageFormat;
	uint32_t m_imageCount;
	std::vector<VkImage> m_images;
	std::vector<VkImageView> m_imageViews;
};

}
