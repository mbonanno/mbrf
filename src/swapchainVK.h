#pragma once

#include "vulkan/vulkan.h"
#include "glfw/glfw3.h"

#include <assert.h>
#include <vector>

#define VK_CHECK(func) { VkResult res = func; assert(res == VK_SUCCESS); }

namespace MBRF
{
	class SwapchainVK
	{
	public:
		void SetInstance(VkInstance instance);
		void SetDevices(VkPhysicalDevice physicalDevice, VkDevice device);

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

		VkInstance m_instance;
		VkPhysicalDevice m_physicalDevice;
		VkDevice m_device;

	public:
		VkSurfaceKHR m_presentationSurface;
		VkExtent2D m_imageExtent;
		VkFormat m_imageFormat;
		uint32_t m_imageCount;
		std::vector<VkImage> m_images;
		std::vector<VkImageView> m_imageViews;
	};
}
