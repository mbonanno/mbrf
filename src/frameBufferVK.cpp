#include "frameBufferVK.h"

#include "deviceVK.h"
#include "Utils.h"

namespace MBRF
{
// ---------------------------- RenderPassCache ----------------------------

std::unordered_map<size_t, VkRenderPass> RenderPassCache::m_renderPasses;

size_t RenderPassCache::GetFrameBufferIndex(const std::vector<TextureViewVK> &attachments)
{
	size_t key = 0;

	for (auto attachment : attachments)
	{
		Utils::HashCombine(key, attachment.GetAspectMask());
		Utils::HashCombine(key, attachment.GetFormat());
		Utils::HashCombine(key, attachment.GetBaseMip());
		Utils::HashCombine(key, attachment.GetMipCount());
	}

	return key;
}

VkRenderPass RenderPassCache::GetRenderPass(DeviceVK* device, const std::vector<TextureViewVK> &attachments)
{
	size_t frameBufferIndex = GetFrameBufferIndex(attachments);

	if (m_renderPasses.find(frameBufferIndex) != m_renderPasses.end())
		return m_renderPasses[frameBufferIndex];

	// TODO: handle multisampling and resolve attachment
	VkRenderPass renderPass = VK_NULL_HANDLE;

	std::vector<VkAttachmentDescription> attachmentDescriptions;
	attachmentDescriptions.resize(attachments.size());

	std::vector<VkAttachmentReference> colorAttachmentRefs;
	VkAttachmentReference depthAttachmentRef;

	// simple validation counter
	uint32_t numDepthAttachments = 0;

	for (size_t i = 0; i < attachments.size(); ++i)
	{
		VkImageAspectFlags aspectMask = attachments[i].GetAspectMask();
		if (aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
		{
			attachmentDescriptions[i].flags = 0;
			attachmentDescriptions[i].format = attachments[i].GetFormat();
			attachmentDescriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachmentDescriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDescriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescriptions[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentDescriptions[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference attachmentRef = { uint32_t(i), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
			colorAttachmentRefs.emplace_back(attachmentRef);
		}
		else if ((aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) && (aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT))
		{
			attachmentDescriptions[i].flags = 0;
			attachmentDescriptions[i].format = attachments[i].GetFormat();
			attachmentDescriptions[i].samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachmentDescriptions[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDescriptions[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachmentDescriptions[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDescriptions[i].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachmentDescriptions[i].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			depthAttachmentRef = { uint32_t(i), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

			numDepthAttachments++;

			// make sure we don't have more than one depth stencil attachment
			assert(numDepthAttachments == 1);
		}
		else
		{
			assert(!"FrameBufferVK::Create: attachments[i].GetAspectMask() type not implemented");

			return false;
		}
	}

	VkSubpassDescription subpassDescriptions[1];
	subpassDescriptions[0].flags = 0;
	subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[0].inputAttachmentCount = 0;
	subpassDescriptions[0].pInputAttachments = nullptr;
	subpassDescriptions[0].colorAttachmentCount = uint32_t(colorAttachmentRefs.size());
	subpassDescriptions[0].pColorAttachments = colorAttachmentRefs.data();
	subpassDescriptions[0].pResolveAttachments = 0;
	subpassDescriptions[0].pDepthStencilAttachment = &depthAttachmentRef;
	subpassDescriptions[0].preserveAttachmentCount = 0;
	subpassDescriptions[0].pPreserveAttachments = nullptr;

	VkSubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;


	VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.attachmentCount = uint32_t(attachmentDescriptions.size());
	createInfo.pAttachments = attachmentDescriptions.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = subpassDescriptions;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(device->GetDevice(), &createInfo, nullptr, &renderPass));

	m_renderPasses[frameBufferIndex] = renderPass;

	return renderPass;
}

void RenderPassCache::Cleanup(DeviceVK* device)
{
	for (auto renderPass : m_renderPasses)
		vkDestroyRenderPass(device->GetDevice(), renderPass.second, nullptr);

	m_renderPasses.clear();
};

// ---------------------------- FrameBufferVK ----------------------------

bool FrameBufferVK::Create(DeviceVK* device, uint32_t width, uint32_t height, const std::vector<TextureViewVK> &attachments)
{
	m_properties.m_width = width;
	m_properties.m_height = height;

	m_attachments.assign(attachments.begin(), attachments.end());

	m_renderPass = RenderPassCache::GetRenderPass(device, attachments);

	std::vector<VkImageView> attachmentImageViews;
	for (auto &attachment: attachments)
		attachmentImageViews.emplace_back(attachment.GetImageView());

	VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.renderPass = m_renderPass;
	createInfo.attachmentCount = uint32_t(attachmentImageViews.size());
	createInfo.pAttachments = attachmentImageViews.data();
	createInfo.width = width;
	createInfo.height = height;
	createInfo.layers = 1;

	VK_CHECK(vkCreateFramebuffer(device->GetDevice(), &createInfo, nullptr, &m_frameBuffer));

	return true;
}

void FrameBufferVK::Destroy(DeviceVK* device)
{
	vkDestroyFramebuffer(device->GetDevice(), m_frameBuffer, nullptr);

	//vkDestroyRenderPass(device->GetDevice(), m_renderPass, nullptr);

	m_frameBuffer = VK_NULL_HANDLE;
	m_renderPass = VK_NULL_HANDLE;
	m_attachments.clear();
}

}
