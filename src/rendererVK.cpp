#include "rendererVK.h"

#include <iostream>
#include <set>

#include <gtc/matrix_transform.hpp>

namespace MBRF
{

bool RendererVK::Init(GLFWwindow* window, uint32_t width, uint32_t height)
{
	m_pendingSwapchainResize = false;
	m_swapchainWidth = width;
	m_swapchainHeight = height;

	// Init Vulkan

	m_device.Init(&m_swapchain, window, width, height, s_maxFramesInFlight);

	// Init Scene/Application
	CreateDepthStencilBuffer();  
	CreateSwapchainFramebuffers(); // swapchain framebuffer
	CreateTextures();
	CreateTestVertexAndTriangleBuffers();

	// wrap
	CreateShaders();  // Scene specific
	CreateUniformBuffers();  // should be scene specific. Currently it is also submitting the descriptors...
	CreateGraphicsPipelines();  // Scene specific

	return true;
}

void RendererVK::Cleanup()
{
	m_device.WaitForDevice();

	DestroyGraphicsPipelines();
	DestroyUniformBuffers();
	DestroyShaders();
	
	DestroyTestVertexAndTriangleBuffers();
	DestroyTextures();
	DestroySwapchainFramebuffers();
	DestroyDepthStencilBuffer();

	RenderPassCache::Cleanup(&m_device);
	SamplerCache::Cleanup(&m_device);

	m_device.Cleanup();
}

void RendererVK::RequestSwapchainResize(uint32_t width, uint32_t height)
{
	m_pendingSwapchainResize = true;
	m_swapchainWidth = width;
	m_swapchainHeight = height;
}

void RendererVK::ResizeSwapchain()
{
	m_device.WaitForDevice();

	DestroySwapchainFramebuffers();
	DestroyDepthStencilBuffer();
	DestroyGraphicsPipelines();

	m_device.RecreateSwapchain(m_swapchainWidth, m_swapchainHeight);

	CreateDepthStencilBuffer();
	CreateSwapchainFramebuffers();
	CreateGraphicsPipelines();

	m_pendingSwapchainResize = false;
}

void RendererVK::Update(double dt)
{
	m_uboTest.m_testColor.x += (float)dt;
	if (m_uboTest.m_testColor.x > 1.0f)
		m_uboTest.m_testColor.x = 0.0f;

	m_testCubeRotation += (float)dt;

	glm::mat4 model = glm::rotate(glm::mat4(1.0f), m_testCubeRotation * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), m_swapchain.m_imageExtent.width / (float)m_swapchain.m_imageExtent.height, 0.1f, 10.0f);

	// Vulkan clip space has inverted Y and half Z.
	glm::mat4 clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);

	m_uboTest.m_mvpTransform = clip * proj * view * model;
}

void RendererVK::Draw()
{
	if (!m_device.BeginFrame())
	{
		ResizeSwapchain();
		return;
	}

	// ------------------ Immediate Context Draw -------------------------------
	ContextVK* context = m_device.GetCurrentGraphicsContext();
	VkCommandBuffer commandBuffer = context->m_commandBuffer;

	context->Begin(&m_device);
	m_device.TransitionImageLayout(commandBuffer, m_swapchain.m_images[m_device.m_currentImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	DrawFrame();

	m_device.TransitionImageLayout(commandBuffer, m_swapchain.m_images[m_device.m_currentImageIndex], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	context->End();
	// ------------------ Immediate Context Draw -------------------------------

	if (!m_device.EndFrame() || m_pendingSwapchainResize)
		ResizeSwapchain();
}

void RendererVK::DrawFrame()
{
	// update uniform buffer (TODO: move in ContextVK?)
	m_uboBuffers[m_device.m_currentImageIndex].Update(&m_device, sizeof(UBOTest), &m_uboTest);


	// TODO: store current swapchain FBO as a global
	FrameBufferVK* currentRenderTarget = &m_swapchainFramebuffers[m_device.m_currentImageIndex];

	ContextVK* context = m_device.GetCurrentGraphicsContext();
	VkCommandBuffer commandBuffer = context->m_commandBuffer;

	context->BeginPass(currentRenderTarget);

	context->ClearRenderTarget(0, 0, m_swapchain.m_imageExtent.width, m_swapchain.m_imageExtent.height, { 0.3f, 0.3f, 0.3f, 1.0f }, { 1.0f, 0 });

	// TODO: need to write PSO wrapper. After replace this with context->SetPSO. Orset individual states + final ApplyState call?
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_testGraphicsPipeline);

	context->SetVertexBuffer(&m_testVertexBuffer, 0);
	context->SetIndexBuffer(&m_testIndexBuffer, 0, false);

	// Draw first test cube
	
	m_pushConstantTestColor.r = 0;
	vkCmdPushConstants(commandBuffer, m_testGraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_pushConstantTestColor), &m_pushConstantTestColor);

	context->SetUniformBuffer(&m_uboBuffers[m_device.m_currentImageIndex], 0);
	context->SetTexture(&m_testTexture, 0);

	context->CommitBindings(&m_device, m_testGraphicsPipelineLayout);

	context->DrawIndexed(sizeof(m_testCubeIndices) / sizeof(uint32_t), 1, 0, 0, 0);

	// Draw second test cube

	m_pushConstantTestColor.r = 1;
	vkCmdPushConstants(commandBuffer, m_testGraphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_pushConstantTestColor), &m_pushConstantTestColor);

	// keep the buffer just bind a different texture
	context->SetTexture(&m_testTexture2, 0);

	context->CommitBindings(&m_device, m_testGraphicsPipelineLayout);

	context->DrawIndexed(sizeof(m_testCubeIndices) / sizeof(uint32_t), 1, 0, 0, 0);

	context->EndPass();
}

bool RendererVK::CreateDepthStencilBuffer()
{
	// TODO: check VK_FORMAT_D24_UNORM_S8_UINT format availability!
	m_depthTexture.Create(&m_device, VK_FORMAT_D24_UNORM_S8_UINT, m_swapchain.m_imageExtent.width, m_swapchain.m_imageExtent.height, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	// Transition (not really needed, we could just set the initial layout to VK_IMAGE_LAYOUT_UNDEFINED in the subpass?)

	m_depthTexture.TransitionImageLayoutAndSubmit(&m_device, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	return true;
}

bool RendererVK::CreateSwapchainFramebuffers()
{
	m_swapchainFramebuffers.resize(m_swapchain.m_imageCount);

	for (size_t i = 0; i < m_swapchainFramebuffers.size(); ++i)
	{
		std::vector<TextureViewVK> attachments = { m_swapchain.m_textureViews[i], m_depthTexture.GetView() };

		m_swapchainFramebuffers[i].Create(&m_device, m_swapchain.m_imageExtent.width, m_swapchain.m_imageExtent.height, attachments);
	}

	return true;
}

void RendererVK::CreateTextures()
{
	m_testTexture.LoadFromFile(&m_device, "data/textures/test.jpg");
	m_testTexture2.LoadFromFile(&m_device, "data/textures/test2.png");
}

bool RendererVK::CreateShaders()
{
	bool result = true;

	// TODO: put common data/shader dir path in a variable or define
	result &= m_device.CreateShaderModuleFromFile("data/shaders/test.vert.spv", m_testVertexShader);
	result &= m_device.CreateShaderModuleFromFile("data/shaders/test.frag.spv", m_testFragmentShader);

	assert(result);

	return result;
}

bool RendererVK::CreateUniformBuffers()
{
	// Create UBO

	m_uboBuffers.resize(m_swapchain.m_imageCount);

	for (size_t i = 0; i < m_uboBuffers.size(); ++i)
	{
		m_uboBuffers[i].Create(&m_device, sizeof(UBOTest), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		m_uboBuffers[i].Update(&m_device, sizeof(UBOTest), &m_uboTest);
	}

	return true;
}

bool RendererVK::CreateGraphicsPipelines()
{
	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2];
	// vertex
	shaderStageCreateInfos[0].sType = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStageCreateInfos[0].pNext = nullptr;
	shaderStageCreateInfos[0].flags = 0;
	shaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageCreateInfos[0].module = m_testVertexShader;
	shaderStageCreateInfos[0].pName = "main";
	shaderStageCreateInfos[0].pSpecializationInfo = nullptr;
	// fragment
	shaderStageCreateInfos[1].sType = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStageCreateInfos[1].pNext = nullptr;
	shaderStageCreateInfos[1].flags = 0;
	shaderStageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageCreateInfos[1].module = m_testFragmentShader;
	shaderStageCreateInfos[1].pName = "main";
	shaderStageCreateInfos[1].pSpecializationInfo = nullptr;

	VkVertexInputBindingDescription bindingDescription[1];
	bindingDescription[0].binding = 0;
	bindingDescription[0].stride = sizeof(TestVertex);
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attributeDescription[3];
	// pos
	attributeDescription[0].location = 0; // shader input location
	attributeDescription[0].binding = 0; // vertex buffer binding
	attributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescription[0].offset = offsetof(TestVertex, pos);
	// color
	attributeDescription[1].location = 1; // shader input location
	attributeDescription[1].binding = 0; // vertex buffer binding
	attributeDescription[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescription[1].offset = offsetof(TestVertex, color);
	// texcoord
	attributeDescription[2].location = 2; // shader input location
	attributeDescription[2].binding = 0; // vertex buffer binding
	attributeDescription[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescription[2].offset = offsetof(TestVertex, texcoord);

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputCreateInfo.pNext = nullptr;
	vertexInputCreateInfo.flags = 0;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = bindingDescription;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = sizeof(attributeDescription) / sizeof(VkVertexInputAttributeDescription);
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescription;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyCreateInfo.pNext = nullptr;
	inputAssemblyCreateInfo.flags = 0;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = { 0, 0, (float)m_swapchain.m_imageExtent.width, (float)m_swapchain.m_imageExtent.height, 0.0f, 1.0f };
	VkRect2D scissor = { 0, 0, m_swapchain.m_imageExtent };

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
	rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
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

	VkPushConstantRange pushConstantRange;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(m_pushConstantTestColor);

	VkDescriptorSetLayout descLayout = m_device.GetDescriptorSetLayout();

	VkPipelineLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layoutCreateInfo.pNext = nullptr;
	layoutCreateInfo.flags = 0;
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &descLayout;
	layoutCreateInfo.pushConstantRangeCount = 1;
	layoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	VK_CHECK(vkCreatePipelineLayout(m_device.GetDevice(), &layoutCreateInfo, nullptr, &m_testGraphicsPipelineLayout));

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.pNext = nullptr;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStageCreateInfos;
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.layout = m_testGraphicsPipelineLayout;
	pipelineCreateInfo.renderPass = m_swapchainFramebuffers[0].GetRenderPass();
	pipelineCreateInfo.subpass = 0;
	// handle of a pipeline to derive from
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	// pass a valid index if the pipeline to derive from is in the same batch of pipelines passed to this vkCreateGraphicsPipelines call
	pipelineCreateInfo.basePipelineIndex = -1;

	VK_CHECK(vkCreateGraphicsPipelines(m_device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_testGraphicsPipeline));

	return true;
}

void RendererVK::CreateTestVertexAndTriangleBuffers()
{
	VkDeviceSize size = sizeof(m_testCubeVerts);

	m_testVertexBuffer.Create(&m_device, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_testVertexBuffer.Update(&m_device, size, m_testCubeVerts);

	// index buffer

	size = sizeof(m_testCubeIndices);

	m_testIndexBuffer.Create(&m_device, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_testIndexBuffer.Update(&m_device, size, m_testCubeIndices);
}

void RendererVK::DestroyShaders()
{
	vkDestroyShaderModule(m_device.GetDevice(), m_testVertexShader, nullptr);
	vkDestroyShaderModule(m_device.GetDevice(), m_testFragmentShader, nullptr);
}

void RendererVK::DestroyGraphicsPipelines()
{
	vkDestroyPipelineLayout(m_device.GetDevice(), m_testGraphicsPipelineLayout, nullptr);
	vkDestroyPipeline(m_device.GetDevice(), m_testGraphicsPipeline, nullptr);
}

void RendererVK::DestroyTestVertexAndTriangleBuffers()
{
	m_testVertexBuffer.Destroy(&m_device);
	m_testIndexBuffer.Destroy(&m_device);
}

void RendererVK::DestroyDepthStencilBuffer()
{
	m_depthTexture.Destroy(&m_device);
}

void RendererVK::DestroyUniformBuffers()
{
	for (size_t i = 0; i < m_uboBuffers.size(); ++i)
		m_uboBuffers[i].Destroy(&m_device);
}

void RendererVK::DestroySwapchainFramebuffers()
{
	for (size_t i = 0; i < m_swapchainFramebuffers.size(); ++i)
		m_swapchainFramebuffers[i].Destroy(&m_device);
}

void RendererVK::DestroyTextures()
{
	m_testTexture.Destroy(&m_device);
	m_testTexture2.Destroy(&m_device);
}

}