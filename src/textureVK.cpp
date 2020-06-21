#include "textureVK.h"

#include "bufferVK.h"
#include "deviceVK.h"
#include "utilsVK.h"
#include "utils.h"

#include <algorithm>

#include <ktx.h>
//#include <ktxvulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace MBRF
{

// ---------------------------- SamplerCache ----------------------------

std::unordered_map<size_t, VkSampler> SamplerCache::m_samplers;

size_t SamplerCache::GetSamplerIndex(VkFilter filter, float minLod, float maxLod)
{
	size_t key;
	Utils::HashCombine(key, filter);
	Utils::HashCombine(key, minLod);
	Utils::HashCombine(key, maxLod);

	return key;
}

VkSampler SamplerCache::GetSampler(DeviceVK* device, VkFilter filter, float minLod, float maxLod)
{
	size_t samplerIndex = GetSamplerIndex(filter, minLod, maxLod);

	if (m_samplers.find(samplerIndex) != m_samplers.end())
		return m_samplers[samplerIndex];

	VkSampler sampler = VK_NULL_HANDLE;

	VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter = filter;
	samplerInfo.minFilter = filter;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	// mipmapping
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = minLod;
	samplerInfo.maxLod = maxLod;

	VK_CHECK(vkCreateSampler(device->GetDevice(), &samplerInfo, nullptr, &sampler));

	m_samplers[samplerIndex] = sampler;

	return sampler;
}

void SamplerCache::Cleanup(DeviceVK* device)
{
	for (auto sampler : m_samplers)
		vkDestroySampler(device->GetDevice(), sampler.second, nullptr);

	m_samplers.clear();
};

// ---------------------------- TextureViewVK ----------------------------

bool TextureViewVK::Create(DeviceVK* device, VkImage image, VkFormat format, VkImageAspectFlags aspectMask, VkImageViewType viewType, uint32_t baseMip, uint32_t mipCount)
{
	assert(m_imageView == VK_NULL_HANDLE);

	VkDevice logicDevice = device->GetDevice();

	m_viewType = viewType;
	m_aspectMask = aspectMask;
	m_baseMip = baseMip;
	m_mipCount = mipCount;
	m_format = format;

	VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCreateInfo.pNext = nullptr;
	imageViewCreateInfo.flags = 0;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = viewType;
	imageViewCreateInfo.format = m_format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	// subresource range
	imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	imageViewCreateInfo.subresourceRange.baseMipLevel = baseMip;
	imageViewCreateInfo.subresourceRange.levelCount = mipCount;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	VK_CHECK(vkCreateImageView(logicDevice, &imageViewCreateInfo, nullptr, &m_imageView));

	return true;
}

bool TextureViewVK::Create(DeviceVK* device, TextureVK* texture, VkImageAspectFlags aspectMask, VkImageViewType viewType, uint32_t baseMip, uint32_t mipCount)
{
	return Create(device, texture->GetImage(), texture->GetFormat(), aspectMask, viewType, baseMip, mipCount);
}

void TextureViewVK::Destroy(DeviceVK* device)
{
	vkDestroyImageView(device->GetDevice(), m_imageView, nullptr);

	m_imageView = VK_NULL_HANDLE;
}

// ---------------------------- TextureVK ----------------------------

bool TextureVK::Create(DeviceVK* device, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mips, VkImageUsageFlags usage,
	VkImageType type, VkImageLayout initialLayout, VkMemoryPropertyFlags memoryProperty, VkSampleCountFlagBits sampleCount, VkImageTiling tiling)
{
	assert(m_image == VK_NULL_HANDLE);

	VkDevice logicDevice = device->GetDevice();
	VkPhysicalDevice physicalDevice = device->GetPhysicalDevice();

	m_imageType = type;
	m_format = format;
	m_width = width;
	m_height = height;
	m_depth = depth;
	m_mips = mips;
	m_sampleCount = sampleCount;
	m_tiling = tiling;
	m_usage = usage;
	m_currentLayout = initialLayout;

	VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.imageType = type;
	createInfo.format = format;
	createInfo.extent.width = width;
	createInfo.extent.height = height;
	createInfo.extent.depth = depth;
	createInfo.mipLevels = mips;
	createInfo.arrayLayers = 1;
	createInfo.samples = sampleCount;
	createInfo.tiling = tiling;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.initialLayout = initialLayout;

	VK_CHECK(vkCreateImage(logicDevice, &createInfo, nullptr, &m_image));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(logicDevice, m_image, &memoryRequirements);

	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

	VkMemoryPropertyFlags memoryProperties = memoryProperty;

	uint32_t memoryType = UtilsVK::FindMemoryType(memoryProperties, memoryRequirements, deviceMemoryProperties);

	assert(memoryType != 0xFFFF);

	if (memoryType == 0xFFFF)
		return false;

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.pNext = nullptr;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryType;

	VK_CHECK(vkAllocateMemory(logicDevice, &allocateInfo, nullptr, &m_memory));

	assert(m_memory != VK_NULL_HANDLE);

	if (!m_memory)
		return false;

	VK_CHECK(vkBindImageMemory(logicDevice, m_image, m_memory, 0));

	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		switch (m_format)
		{
		case VK_FORMAT_D16_UNORM:
		case VK_FORMAT_X8_D24_UNORM_PACK32:
		case VK_FORMAT_D32_SFLOAT:

			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			break;
		case VK_FORMAT_S8_UINT:

			aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
		default:

			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			break;
		}
	}

	m_view.Create(device, this, aspectMask, VK_IMAGE_VIEW_TYPE_2D, 0, m_mips);

	m_sampler = SamplerCache::GetSampler(device, VK_FILTER_LINEAR, 0.0f, float(mips));

	UpdateDescriptor();

	return true;
}

bool TextureVK::Update(DeviceVK* device, VkDeviceSize size, VkImageLayout newLayout, void* data, std::vector<VkBufferImageCopy> regions)
{
	if (!(m_usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
	{
		assert(!"Cannot upload data to device local memory via staging buffer. Usage needs to be VK_BUFFER_USAGE_TRANSFER_DST_BIT.");
		return false;
	}

	TransitionImageLayoutAndSubmit(device, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	BufferVK stagingBuffer;
	stagingBuffer.Create(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	std::memcpy(stagingBuffer.GetData(), data, size);

	VkCommandBuffer commandBuffer = device->BeginNewCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.GetBuffer(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uint32_t(regions.size()), regions.data());

	device->SubmitCommandBufferAndWait(commandBuffer, true);

	TransitionImageLayoutAndSubmit(device, newLayout);

	stagingBuffer.Destroy(device);

	return true;
}

// NOTE: this only updates mip0
bool TextureVK::Update(DeviceVK* device, uint32_t width, uint32_t height, uint32_t depth, VkDeviceSize size, VkImageLayout newLayout, void* data)
{
	std::vector<VkBufferImageCopy> regions(1);

	regions[0].bufferOffset = 0;
	regions[0].bufferRowLength = 0;
	regions[0].bufferImageHeight = 0;
	regions[0].imageSubresource.aspectMask = m_view.GetAspectMask();
	regions[0].imageSubresource.mipLevel = 0;
	regions[0].imageSubresource.baseArrayLayer = 0;
	regions[0].imageSubresource.layerCount = 1;
	regions[0].imageOffset = { 0, 0, 0 };
	regions[0].imageExtent.width = width;
	regions[0].imageExtent.height = height;
	regions[0].imageExtent.depth = depth;

	return Update(device, size, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, data, regions);
}

void TextureVK::Destroy(DeviceVK* device)
{
	VkDevice logicDevice = device->GetDevice();

	m_view.Destroy(device);

	vkFreeMemory(logicDevice, m_memory, nullptr);
	vkDestroyImage(logicDevice, m_image, nullptr);

	m_image = VK_NULL_HANDLE;
	m_memory = VK_NULL_HANDLE;
}

void TextureVK::LoadFromFile(DeviceVK* device, const char* fileName)
{
	int texWidth, texHeight, texChannels;
	
	stbi_uc* pixels = stbi_load(fileName, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	Create(device, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, 1, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	// upload image pixels to the GPU

	Update(device, texWidth, texHeight, 1, imageSize, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, pixels);

	stbi_image_free(pixels);
}

void TextureVK::LoadFromKTXFile(DeviceVK* device, const char* fileName, VkFormat format)
{
	// TODO: figure out the format automatically?
	ktxResult result;
	ktxTexture* ktxTexture;

	result = ktxTexture_CreateFromNamedFile(fileName, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

	ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
	uint32_t texWidth = ktxTexture->baseWidth;
	uint32_t texHeight = ktxTexture->baseHeight;
	uint32_t texDepth = ktxTexture->baseDepth;
	uint32_t bpp = ktxTexture_GetElementSize(ktxTexture);
	uint32_t mipLevels = ktxTexture->numLevels;
	ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

	assert(result == KTX_SUCCESS);

	VkDeviceSize imageSize = ktxTextureSize;

	Create(device, format, texWidth, texHeight, texDepth, mipLevels, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	// upload image pixels to the GPU

	std::vector<VkBufferImageCopy> regions;

	for (uint32_t mipLevel = 0; mipLevel < m_mips; ++mipLevel)
	{
		ktx_size_t offset = 0;
		KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, mipLevel, 0, 0, &offset);
		assert(ret == KTX_SUCCESS);

		VkBufferImageCopy region;
		region.bufferOffset = offset;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = m_view.GetAspectMask();
		region.imageSubresource.mipLevel = mipLevel;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent.width = std::max(int(texWidth >> mipLevel), 1);
		region.imageExtent.height = std::max(int(texHeight >> mipLevel), 1);
		region.imageExtent.depth = std::max(int(texDepth >> mipLevel), 1);

		regions.emplace_back(region);
	}

	Update(device, imageSize, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, ktxTextureData, regions);

	ktxTexture_Destroy(ktxTexture);
}

void TextureVK::TransitionImageLayout(DeviceVK* device, VkCommandBuffer commandBuffer, VkImageLayout newLayout)
{
	device->TransitionImageLayout(commandBuffer, m_image, m_view.GetAspectMask(), m_currentLayout, newLayout);

	m_currentLayout = newLayout;

	UpdateDescriptor();
}

void TextureVK::TransitionImageLayoutAndSubmit(DeviceVK* device, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = device->BeginNewCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	TransitionImageLayout(device, commandBuffer, newLayout);

	device->SubmitCommandBufferAndWait(commandBuffer, true);
}

void TextureVK::UpdateDescriptor()
{
	m_descriptor.sampler = m_sampler;
	m_descriptor.imageView = m_view.GetImageView();
	m_descriptor.imageLayout = m_currentLayout;
}

}