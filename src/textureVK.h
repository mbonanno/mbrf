#pragma once

#include "commonVK.h"

namespace MBRF
{

class DeviceVK;
class TextureVK;

class SamplerVK
{
public:
	bool Create(DeviceVK* device, VkFilter filter, float minLod, float maxLod);
	void Destroy(DeviceVK* device);

	const VkSampler& GetSampler() const { return m_sampler; };

private:
	VkSampler m_sampler;
};

class TextureViewVK
{
public:
	bool Create(DeviceVK* device, TextureVK* texture, VkImageAspectFlags aspectMask, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t baseMip = 0, uint32_t mipCount = 1);
	void Destroy(DeviceVK* device);

	const VkImageView& GetImageView() const { return m_imageView; };
	VkImageAspectFlags GetAspectMask() const { return m_aspectMask; };
	uint32_t GetBaseMip() const { return m_baseMip; };
	uint32_t GetMipCount() const { return m_mipCount; };

private:
	VkImageView m_imageView = VK_NULL_HANDLE;

	VkImageViewType m_viewType;
	VkImageAspectFlags m_aspectMask;
	uint32_t m_baseMip;
	uint32_t m_mipCount;
};

class TextureVK
{
public:
	// TODO: add a desc param?
	bool Create(DeviceVK* device, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mips = 1, VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VkImageType type = VK_IMAGE_TYPE_2D, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, VkMemoryPropertyFlags memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);

	bool Update(DeviceVK* device, uint32_t width, uint32_t height, uint32_t depth, uint32_t bpp, VkImageLayout newLayout, void* data);
	void Destroy(DeviceVK* device);

	void LoadFromFile(DeviceVK* device, const char* fileName);

	const VkImage& GetImage() const { return m_image; };
	const TextureViewVK& GetView() const { return m_view; };
	VkFormat GetFormat() const { return m_format; };
	VkImageUsageFlags GetUsage() { return m_usage; };

	VkImageLayout GetCurrentLayout() { return m_currentLayout; };

	const VkDescriptorImageInfo& GetDescriptor() const { return m_descriptor; };

	void TransitionImageLayoutAndSubmit(DeviceVK* device, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout);

private:
	void UpdateDescriptor();

private:
	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;

	VkDescriptorImageInfo m_descriptor;

	TextureViewVK m_view;
	// TODO: decouple from the sampler? Could just have some global samplers
	SamplerVK m_sampler;

	VkImageType m_imageType;
	VkFormat m_format;
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint32_t m_depth = 0;
	uint32_t m_mips = 1;

	VkSampleCountFlagBits m_sampleCount;
	VkImageTiling m_tiling;
	VkImageUsageFlags m_usage;
	VkImageLayout m_currentLayout;
};

}
