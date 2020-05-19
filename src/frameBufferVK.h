#pragma once

#include "commonVK.h"
#include "textureVK.h"

#include <unordered_map>

namespace MBRF
{

class DeviceVK;

class RenderPassCache
{
public:
	static VkRenderPass GetRenderPass(DeviceVK* device, const std::vector<TextureViewVK> &attachments);
	static void Cleanup(DeviceVK* device);

	static size_t GetFrameBufferIndex(const std::vector<TextureViewVK> &attachments);

	

	// possible different way to get a std::map key. Can't use with std::unordered_map 

	//struct AttachmentParams
	//{
	//	uint32_t       numTargets;
	//	VkFormat       colorFormat;
	//	VkFormat       depthFormat;
	//
	//	bool operator <(const AttachmentParams &rval) const
	//	{
	//		return
	//			std::tie(numTargets,
	//				colorFormat,
	//				depthFormat)
	//			<
	//			std::tie(rval.numTargets,
	//				rval.colorFormat,
	//				rval.depthFormat);
	//	}
	//};
	//
	//struct FrameBufferIndexV
	//{
	//	std::vector<AttachmentParams> gender;
	//};

private:
	static std::unordered_map<size_t, VkRenderPass> m_renderPasses;
};

struct FrameBufferProperties
{
	uint32_t m_width;
	uint32_t m_height;
};

class FrameBufferVK
{
public:
	bool Create(DeviceVK* device, uint32_t width, uint32_t height, const std::vector<TextureViewVK> &attachments);
	void Destroy(DeviceVK* device);

	const VkFramebuffer& GetFrameBuffer() const { return m_frameBuffer; };
	const VkRenderPass& GetRenderPass() const { return m_renderPass; };
	const std::vector<TextureViewVK>& GetAttachments() const { return m_attachments; };

	const FrameBufferProperties& GetProperties() const { return m_properties; };

private:
	VkFramebuffer m_frameBuffer = VK_NULL_HANDLE;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;

	std::vector<TextureViewVK> m_attachments;

	FrameBufferProperties m_properties;
};

}
