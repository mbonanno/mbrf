#pragma once

#include "commonVK.h"
#include "resource.h"

#include <unordered_map>

namespace MBRF
{

class DeviceVK;
class TextureVK;

class SamplerCache
{
public:
	static VkSampler GetSampler(DeviceVK* device, VkFilter filter, float minLod, float maxLod);
	static void Cleanup(DeviceVK* device);

	static size_t GetSamplerIndex(VkFilter filter, float minLod, float maxLod);

private:
	static std::unordered_map<size_t, VkSampler> m_samplers;
};

class TextureViewVK
{
public:
	bool Create(DeviceVK* device, VkImage image, VkFormat format, VkImageAspectFlags aspectMask, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t baseMip = 0, uint32_t mipCount = 1, uint32_t numLayers = 1);
	bool Create(DeviceVK* device, TextureVK* texture, VkImageAspectFlags aspectMask, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t baseMip = 0, uint32_t mipCount = 1, uint32_t numLayers = 1);
	void Destroy(DeviceVK* device);

	const VkImageView GetImageView() const { return m_imageView; };
	VkFormat GetFormat() const { return m_format; };
	VkImageAspectFlags GetAspectMask() const { return m_aspectMask; };
	uint32_t GetBaseMip() const { return m_baseMip; };
	uint32_t GetMipCount() const { return m_mipCount; };

private:
	VkImageView m_imageView = VK_NULL_HANDLE;

	VkFormat m_format;
	VkImageViewType m_viewType;
	VkImageAspectFlags m_aspectMask;
	uint32_t m_baseMip;
	uint32_t m_mipCount;
};

class TextureVK : public Resource
{
public:
	// TODO: add a desc param?
	bool Create(DeviceVK* device, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mips = 1, VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, uint32_t layers=1,
		bool cubemap = false, VkImageType type = VK_IMAGE_TYPE_2D, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, VkMemoryPropertyFlags memoryProperty = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);

	bool Update(DeviceVK* device, VkDeviceSize size, VkImageLayout newLayout, void* data, std::vector<VkBufferImageCopy> regions);
	bool Update(DeviceVK* device, uint32_t width, uint32_t height, uint32_t depth, VkDeviceSize size, VkImageLayout newLayout, void* data);
	void Destroy(DeviceVK* device);

	void LoadFromFile(DeviceVK* device, const char* fileName);
	void LoadFromKTXFile(DeviceVK* device, const char* fileName, VkFormat format);

	const VkImage GetImage() const { return m_image; };
	const TextureViewVK GetView() const { return m_view; };
	VkFormat GetFormat() const { return m_format; };
	VkImageUsageFlags GetUsage() { return m_usage; };

	VkImageLayout GetCurrentLayout() { return m_currentLayout; };

	const VkDescriptorImageInfo& GetDescriptor() const { return m_descriptor; };

	void TransitionImageLayout(DeviceVK* device, VkCommandBuffer commandBuffer, VkImageLayout newLayout);
	void TransitionImageLayoutAndSubmit(DeviceVK* device, VkImageLayout newLayout);

	uint32_t GetWidth() const { return m_width; };
	uint32_t GetHeight() const { return m_height; };

private:
	void UpdateDescriptor();

private:
	VkImage m_image = VK_NULL_HANDLE;
	VkDeviceMemory m_memory = VK_NULL_HANDLE;

	VkDescriptorImageInfo m_descriptor;

	TextureViewVK m_view;
	// TODO: decouple from the sampler? Could just have some global samplers
	VkSampler m_sampler;

	VkImageType m_imageType;
	VkFormat m_format;
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	uint32_t m_depth = 0;
	uint32_t m_mips = 1;
	uint32_t m_layers = 1;

	VkSampleCountFlagBits m_sampleCount;
	VkImageTiling m_tiling;
	VkImageUsageFlags m_usage;
	VkImageLayout m_currentLayout;

	bool m_isCubemap = false;
	bool m_isArray = false;
};

}
