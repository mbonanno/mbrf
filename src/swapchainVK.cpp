#include "swapchainVK.h"

#include <algorithm>

namespace MBRF
{

void SwapchainVK::SetInstance(VkInstance instance)
{
	m_instance = instance;
}

void SwapchainVK::SetDevices(VkPhysicalDevice physicalDevice, VkDevice device)
{
	m_physicalDevice = physicalDevice;
	m_device = device;
}

bool SwapchainVK::CreatePresentationSurface(GLFWwindow* window)
{
	assert(m_instance);

	m_presentationSurface = VK_NULL_HANDLE;
	VK_CHECK(glfwCreateWindowSurface(m_instance, window, nullptr, &m_presentationSurface));

	assert(m_presentationSurface != VK_NULL_HANDLE);

	return m_presentationSurface != VK_NULL_HANDLE;
}

void SwapchainVK::DestroyPresentationSurface()
{
	vkDestroySurfaceKHR(m_instance, m_presentationSurface, nullptr);
}

bool SwapchainVK::Create(uint32_t width, uint32_t height)
{
	assert(m_physicalDevice && m_device);

	// set presentation mode

	VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;  // always supported

	uint32_t presentModesCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_presentationSurface, &presentModesCount, nullptr));

	assert(presentModesCount > 0);

	if (presentModesCount == 0)
		return false;

	std::vector<VkPresentModeKHR> supportedPresentModes;
	supportedPresentModes.resize(presentModesCount);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_presentationSurface, &presentModesCount, supportedPresentModes.data()));

	for (auto currentPresentMode : supportedPresentModes)
	{
		if (currentPresentMode == desiredPresentMode)
		{
			presentMode = currentPresentMode;
			break;
		}
	}

	assert(presentMode == desiredPresentMode);

	// set number of images

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_presentationSurface, &surfaceCapabilities));

	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0)
		imageCount = std::min(imageCount, surfaceCapabilities.maxImageCount);

	// set swapchain images size

	// check if the window's size is determined by the swapchain images size
	if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF)
	{
		VkExtent2D minImageSize = surfaceCapabilities.minImageExtent;
		VkExtent2D maxImageSize = surfaceCapabilities.maxImageExtent;

		m_imageExtent.width = std::min(std::max(width, minImageSize.width), maxImageSize.width);
		m_imageExtent.height = std::min(std::max(height, minImageSize.height), maxImageSize.height);
	}
	else
	{
		m_imageExtent = surfaceCapabilities.currentExtent;
	}

	// set images usage

	VkImageUsageFlags desiredUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;  // this usage is always guaranteed. so no real need to check. Adding the check for completeness
	VkImageUsageFlags imageUsage = surfaceCapabilities.supportedUsageFlags & desiredUsage;

	assert(imageUsage == desiredUsage);

	if (imageUsage != desiredUsage)
		return false;

	// set transform (not really required on PC, adding the code for completeness)

	VkSurfaceTransformFlagBitsKHR desiredTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	VkSurfaceTransformFlagBitsKHR surfaceTransform;

	if (desiredTransform & surfaceCapabilities.supportedTransforms)
		surfaceTransform = desiredTransform;
	else
		surfaceTransform = surfaceCapabilities.currentTransform;

	// Set format

	VkSurfaceFormatKHR desiredFormat = { VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	m_imageFormat = VK_FORMAT_UNDEFINED;
	VkColorSpaceKHR imageColorSpace;

	uint32_t formatsCount;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_presentationSurface, &formatsCount, nullptr));

	assert(formatsCount > 0);

	if (formatsCount == 0)
		return false;

	std::vector<VkSurfaceFormatKHR> supportedFormats;
	supportedFormats.resize(formatsCount);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_presentationSurface, &formatsCount, supportedFormats.data()));

	// if the only element has VK_FORMAT_UNDEFINED then we are free to choose whatever format and colorspace we want
	if (formatsCount == 1 && supportedFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		m_imageFormat = desiredFormat.format;
		imageColorSpace = desiredFormat.colorSpace;
	}
	else
	{
		// look for format + colorspace match
		for (auto format : supportedFormats)
		{
			if (format.format == desiredFormat.format && format.colorSpace == desiredFormat.colorSpace)
			{
				m_imageFormat = desiredFormat.format;
				imageColorSpace = desiredFormat.colorSpace;

				break;
			}
		}

		// if a format + colorspace match wasn't found, look for only the matching format, and accept whatever colorspace is supported with that format
		if (m_imageFormat == VK_FORMAT_UNDEFINED)
		{
			for (auto format : supportedFormats)
			{
				if (format.format == desiredFormat.format)
				{
					m_imageFormat = desiredFormat.format;
					imageColorSpace = format.colorSpace;

					break;
				}
			}
		}

		// if a format match wasn't found, accept the first format and colorspace supported available
		if (m_imageFormat == VK_FORMAT_UNDEFINED)
		{
			m_imageFormat = supportedFormats[0].format;
			imageColorSpace = supportedFormats[0].colorSpace;
		}
	}

	assert(m_imageFormat != VK_FORMAT_UNDEFINED);

	// Create Swapchain

	VkSwapchainCreateInfoKHR swapchainCreateInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = m_presentationSurface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = m_imageFormat;
	swapchainCreateInfo.imageColorSpace = imageColorSpace;
	swapchainCreateInfo.imageExtent = m_imageExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = imageUsage;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;  // number of queue families having access to the image(s) of the swapchain when imageSharingMode is VK_SHARING_MODE_CONCURRENT
	swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	swapchainCreateInfo.preTransform = surfaceTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VK_CHECK(vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain));

	// Get handles of swapchain images

	// query imageCount again, as we only required the min number of images. The driver might have created more
	VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, nullptr));

	m_images.resize(m_imageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, m_images.data()));

	return CreateImageViews();
}

void SwapchainVK::Cleanup()
{
	DestroyImageViews();

	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

bool SwapchainVK::CreateImageViews()
{
	size_t imageCount = m_images.size();

	m_imageViews.resize(imageCount);

	for (size_t i = 0; i < imageCount; ++i)
	{
		VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		imageViewCreateInfo.pNext = nullptr;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = m_images[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = m_imageFormat;
		// subresource range
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VK_CHECK(vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &m_imageViews[i]));
	}

	return true;
}

void SwapchainVK::DestroyImageViews()
{
	for (size_t i = 0; i < m_imageViews.size(); ++i)
	{
		vkDestroyImageView(m_device, m_imageViews[i], nullptr);
	}
}

uint32_t SwapchainVK::AcquireNextImage(VkSemaphore semaphore)
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, semaphore, nullptr, &imageIndex);

	assert(result == VK_SUCCESS);

	switch (result)
	{
	case VK_SUCCESS:
	case VK_SUBOPTIMAL_KHR:
		return imageIndex;
	default:
		return UINT32_MAX;
	}
}

bool SwapchainVK::PresentQueue(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore)
{
	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &waitSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));

	return true;
}

}