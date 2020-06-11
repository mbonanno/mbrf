#include "pipelineVK.h"

#include "deviceVK.h"
#include "frameBufferVK.h"
#include "vertexFormatVK.h"

namespace MBRF
{

// ------------------------------- PipelineVK -------------------------------

void PipelineVK::Destroy(DeviceVK* device)
{
	vkDestroyPipeline(device->GetDevice(), m_pipeline, nullptr);
}

VkPipelineBindPoint PipelineVK::GetBindPoint()
{
	switch (m_type)
	{
	case PipelineVK::PIPELINE_TYPE_GRAPHICS:
		return VK_PIPELINE_BIND_POINT_GRAPHICS;

	case PipelineVK::PIPELINE_TYPE_COMPUTE:
		return VK_PIPELINE_BIND_POINT_COMPUTE;

	default:
		assert(0); // not implemented
		return VK_PIPELINE_BIND_POINT_MAX_ENUM;
	}
}

// ------------------------------- GraphicsPipelineVK -------------------------------

VkCullModeFlags CullModeToVk[NUM_CULL_MODES] = 
{
	VK_CULL_MODE_BACK_BIT,
	VK_CULL_MODE_FRONT_BIT,
	VK_CULL_MODE_NONE
};

bool GraphicsPipelineVK::Create(DeviceVK* device, const GraphicsPipelineDesc &desc)
{
	std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;

	for (auto shader: desc.m_shaders)
	{
		VkPipelineShaderStageCreateInfo pssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

		pssci.pNext = nullptr;
		pssci.flags = 0;
		pssci.stage = shader.GetStage();
		pssci.module = shader.GetShaderModule();
		pssci.pName = "main";
		pssci.pSpecializationInfo = nullptr;

		shaderStageCreateInfos.emplace_back(pssci);
	}

	VkVertexInputBindingDescription bindingDescription[] = { desc.m_vertexFormat->GetBindingDescription() };
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions = desc.m_vertexFormat->GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputCreateInfo.pNext = nullptr;
	vertexInputCreateInfo.flags = 0;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = bindingDescription;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = uint32_t(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyCreateInfo.pNext = nullptr;
	inputAssemblyCreateInfo.flags = 0;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	uint32_t width = desc.m_frameBuffer->GetWidth();
	uint32_t height = desc.m_frameBuffer->GetHeight();

	VkViewport viewport = { 0, 0, float(width), float(height), 0.0f, 1.0f };
	VkExtent2D scissorExtent = { width , height };
	VkRect2D scissor = { 0, 0, scissorExtent };

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportStateCreateInfo.pNext = nullptr;
	viewportStateCreateInfo.flags = 0;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationCreateInfo.pNext = nullptr;
	rasterizationCreateInfo.flags = 0;
	rasterizationCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationCreateInfo.cullMode = CullModeToVk[desc.m_cullMode];
	rasterizationCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationCreateInfo.depthBiasClamp = 0.0f;
	rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationCreateInfo.lineWidth = 1.0f;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilStateCreateInfo.pNext = nullptr;
	depthStencilStateCreateInfo.flags = 0;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateCreateInfo.front = {};
	depthStencilStateCreateInfo.back = {};
	depthStencilStateCreateInfo.minDepthBounds = 0.0f;
	depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleCreateInfo.pNext = nullptr;
	multisampleCreateInfo.flags = 0;
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleCreateInfo.minSampleShading = 1.0f;
	multisampleCreateInfo.pSampleMask = nullptr;
	multisampleCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState blendAttachmentState;
	blendAttachmentState.blendEnable = VK_FALSE;
	blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendCreateInfo.pNext = nullptr;
	colorBlendCreateInfo.flags = 0;
	colorBlendCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendCreateInfo.attachmentCount = 1;
	colorBlendCreateInfo.pAttachments = &blendAttachmentState;
	colorBlendCreateInfo.blendConstants[0] = 0.0f;
	colorBlendCreateInfo.blendConstants[1] = 0.0f;
	colorBlendCreateInfo.blendConstants[2] = 0.0f;
	colorBlendCreateInfo.blendConstants[3] = 0.0f;

	//VkPushConstantRange pushConstantRange;
	//pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	//pushConstantRange.offset = 0;
	//pushConstantRange.size = sizeof(m_pushConstantTestColor);

	VkDescriptorSetLayout descLayout = device->GetDescriptorSetLayout();

	VkPipelineLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutCreateInfo.pNext = nullptr;
	layoutCreateInfo.flags = 0;
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &descLayout;
	layoutCreateInfo.pushConstantRangeCount = 0; // 1;
	layoutCreateInfo.pPushConstantRanges = nullptr; // &pushConstantRange;

	VK_CHECK(vkCreatePipelineLayout(device->GetDevice(), &layoutCreateInfo, nullptr, &m_layout));

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.pNext = nullptr;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.stageCount = uint32_t(shaderStageCreateInfos.size());
	pipelineCreateInfo.pStages = shaderStageCreateInfos.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.layout = m_layout;
	pipelineCreateInfo.renderPass = desc.m_frameBuffer->GetRenderPass();
	pipelineCreateInfo.subpass = 0;
	// handle of a pipeline to derive from
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	// pass a valid index if the pipeline to derive from is in the same batch of pipelines passed to this vkCreateGraphicsPipelines call
	pipelineCreateInfo.basePipelineIndex = -1;

	VK_CHECK(vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline));

	return true;
}

void GraphicsPipelineVK::Destroy(DeviceVK* device)
{
	vkDestroyPipelineLayout(device->GetDevice(), m_layout, nullptr);
	PipelineVK::Destroy(device);
}

// ------------------------------- ComputePipelineVK -------------------------------

bool ComputePipelineVK::Create(DeviceVK* device, ShaderVK* computeShader)
{
	assert(computeShader->GetStage() == VK_SHADER_STAGE_COMPUTE_BIT);

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStageCreateInfo.pNext = nullptr;
	shaderStageCreateInfo.flags = 0;
	shaderStageCreateInfo.stage = computeShader->GetStage();
	shaderStageCreateInfo.module = computeShader->GetShaderModule();
	shaderStageCreateInfo.pName = "main";
	shaderStageCreateInfo.pSpecializationInfo = nullptr;

	VkDescriptorSetLayout descLayout = device->GetDescriptorSetLayout();

	VkPipelineLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutCreateInfo.pNext = nullptr;
	layoutCreateInfo.flags = 0;
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &descLayout;
	layoutCreateInfo.pushConstantRangeCount = 0;
	layoutCreateInfo.pPushConstantRanges = nullptr;

	VK_CHECK(vkCreatePipelineLayout(device->GetDevice(), &layoutCreateInfo, nullptr, &m_layout));

	VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.stage = shaderStageCreateInfo;
	createInfo.layout = m_layout;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.basePipelineIndex = -1;

	VK_CHECK(vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline));

	return true;
}

void ComputePipelineVK::Destroy(DeviceVK* device)
{
	vkDestroyPipelineLayout(device->GetDevice(), m_layout, nullptr);
	PipelineVK::Destroy(device);
}

}
