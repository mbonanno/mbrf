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
	CreateBackBuffer(); // swapchain framebuffer
	CreateTextures();
	CreateTestVertexAndTriangleBuffers();

	// wrap
	CreateShaders();  // Scene specific
	CreateUniformBuffers();  // should be scene specific. Currently it is also submitting the descriptors...
	
	ShaderVK shaders[] = { m_testVertexShader, m_testFragmentShader };
	CreateGraphicsPipeline(&m_backBuffer.m_frameBuffers[0], shaders);

	return true;
}

void RendererVK::Cleanup()
{
	m_device.WaitForDevice();

	DestroyGraphicsPipeline();
	DestroyUniformBuffers();
	DestroyShaders();
	
	DestroyTestVertexAndTriangleBuffers();
	DestroyTextures();
	DestroyBackBuffer();

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

	DestroyBackBuffer();
	DestroyGraphicsPipeline();

	m_device.RecreateSwapchain(m_swapchainWidth, m_swapchainHeight);

	CreateBackBuffer();

	ShaderVK shaders[] = { m_testVertexShader, m_testFragmentShader };
	CreateGraphicsPipeline(&m_backBuffer.m_frameBuffers[0], shaders);

	m_pendingSwapchainResize = false;
}

void RendererVK::Update(double dt)
{
	m_uboTest.m_testColor.x += (float)dt;
	if (m_uboTest.m_testColor.x > 1.0f)
		m_uboTest.m_testColor.x = 0.0f;

	m_testCubeRotation += (float)dt;

	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), m_testCubeRotation * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), m_backBuffer.m_frameBuffers[0].GetWidth() / float(m_backBuffer.m_frameBuffers[0].GetHeight()), 0.1f, 10.0f);

	// Vulkan clip space has inverted Y and half Z.
	glm::mat4 clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f);

	m_uboTest.m_mvpTransform = clip * proj * view * model;

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), m_testCubeRotation * glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	m_uboTest2.m_mvpTransform = clip * proj * view * model;
}

void RendererVK::Draw()
{
	if (!m_device.BeginFrame())
	{
		ResizeSwapchain();
		return;
	}

	uint32_t currentFrameIndex = m_device.m_currentImageIndex;
	VkImage currentSwapchainImage = m_swapchain.m_images[currentFrameIndex];

	// ------------------ Immediate Context Draw -------------------------------
	ContextVK* context = m_device.GetCurrentGraphicsContext();
	VkCommandBuffer commandBuffer = context->m_commandBuffer;

	context->Begin(&m_device);
	m_device.TransitionImageLayout(commandBuffer, currentSwapchainImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	DrawFrame(currentFrameIndex);

	m_device.TransitionImageLayout(commandBuffer, currentSwapchainImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	context->End();
	// ------------------ Immediate Context Draw -------------------------------

	if (!m_device.EndFrame() || m_pendingSwapchainResize)
		ResizeSwapchain();
}

void RendererVK::DrawFrame(uint32_t currentFrameIndex)
{
	// update uniform buffer (TODO: move in ContextVK?)
	m_uboBuffers1.Update(&m_device, currentFrameIndex, &m_uboTest);
	m_uboBuffers2.Update(&m_device, currentFrameIndex, &m_uboTest2);

	// TODO: store current swapchain FBO as a global
	FrameBufferVK* currentRenderTarget = &m_backBuffer.m_frameBuffers[currentFrameIndex];

	ContextVK* context = m_device.GetCurrentGraphicsContext();
	VkCommandBuffer commandBuffer = context->m_commandBuffer;

	context->BeginPass(currentRenderTarget);

	context->ClearRenderTarget(0, 0, currentRenderTarget->GetWidth(), currentRenderTarget->GetHeight(), { 0.3f, 0.3f, 0.3f, 1.0f }, { 1.0f, 0 });

	// TODO: need to write PSO wrapper. After replace this with context->SetPSO. Orset individual states + final ApplyState call?
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_testGraphicsPipeline);

	context->SetVertexBuffer(&m_testVertexBuffer, 0);
	context->SetIndexBuffer(&m_testIndexBuffer, 0);

	// Draw first test cube

	context->SetUniformBuffer(&m_uboBuffers1.GetBuffer(currentFrameIndex), 0);
	context->SetTexture(&m_testTexture, 0);

	context->CommitBindings(&m_device, m_testGraphicsPipelineLayout);

	context->DrawIndexed(m_testIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	// Draw second test cube

	context->SetUniformBuffer(&m_uboBuffers2.GetBuffer(currentFrameIndex), 0);
	context->SetTexture(&m_testTexture2, 0);

	context->CommitBindings(&m_device, m_testGraphicsPipelineLayout);

	context->DrawIndexed(m_testIndexBuffer.GetNumIndices(), 1, 0, 0, 0);

	context->EndPass();
}

bool RendererVK::CreateBackBuffer()
{
	// Create Depth Stencil

	// TODO: check VK_FORMAT_D24_UNORM_S8_UINT format availability!
	m_backBuffer.m_depthStencilBuffer.Create(&m_device, VK_FORMAT_D24_UNORM_S8_UINT, m_swapchain.m_imageExtent.width, m_swapchain.m_imageExtent.height, 1, 1, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

	// Transition (not really needed, we could just set the initial layout to VK_IMAGE_LAYOUT_UNDEFINED in the subpass?)
	m_backBuffer.m_depthStencilBuffer.TransitionImageLayoutAndSubmit(&m_device, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	// Create Framebuffers

	m_backBuffer.m_frameBuffers.resize(m_swapchain.m_imageCount);

	for (size_t i = 0; i < m_backBuffer.m_frameBuffers.size(); ++i)
	{
		std::vector<TextureViewVK> attachments = { m_swapchain.m_textureViews[i], m_backBuffer.m_depthStencilBuffer.GetView() };

		m_backBuffer.m_frameBuffers[i].Create(&m_device, m_swapchain.m_imageExtent.width, m_swapchain.m_imageExtent.height, attachments);
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
	result &= m_testVertexShader.CreateFromFile(&m_device, "data/shaders/test.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	result &= m_testFragmentShader.CreateFromFile(&m_device, "data/shaders/test.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	assert(result);

	return result;
}

bool RendererVK::CreateUniformBuffers()
{
	m_uboBuffers1.Create(&m_device, m_swapchain.m_imageCount, sizeof(UBOTest));
	m_uboBuffers2.Create(&m_device, m_swapchain.m_imageCount, sizeof(UBOTest));

	return true;
}

bool RendererVK::CreateGraphicsPipeline(FrameBufferVK* frameBuffer, ShaderVK* shaders)
{
	VkPipelineShaderStageCreateInfo shaderStageCreateInfos[2];
	// vertex
	shaderStageCreateInfos[0].sType = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStageCreateInfos[0].pNext = nullptr;
	shaderStageCreateInfos[0].flags = 0;
	shaderStageCreateInfos[0].stage = shaders[0].GetStage();
	shaderStageCreateInfos[0].module = shaders[0].GetShaderModule();
	shaderStageCreateInfos[0].pName = "main";
	shaderStageCreateInfos[0].pSpecializationInfo = nullptr;
	// fragment
	shaderStageCreateInfos[1].sType = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStageCreateInfos[1].pNext = nullptr;
	shaderStageCreateInfos[1].flags = 0;
	shaderStageCreateInfos[1].stage = shaders[1].GetStage();
	shaderStageCreateInfos[1].module = shaders[1].GetShaderModule();
	shaderStageCreateInfos[1].pName = "main";
	shaderStageCreateInfos[1].pSpecializationInfo = nullptr;

	VkVertexInputBindingDescription bindingDescription[] = { TestVertex::GetBindingDescription() };
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions = TestVertex::GetAttributeDescriptions();

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

	VkViewport viewport = { 0, 0, float(frameBuffer->GetWidth()), float(frameBuffer->GetHeight()), 0.0f, 1.0f };
	VkExtent2D scissorExtent = { frameBuffer->GetWidth() , frameBuffer->GetHeight() };
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
	pipelineCreateInfo.renderPass = frameBuffer->GetRenderPass();
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
	uint32_t size = sizeof(m_testCubeVerts);

	m_testVertexBuffer.Create(&m_device, size, m_testCubeVerts);

	// index buffer

	size = sizeof(m_testCubeIndices);

	m_testIndexBuffer.Create(&m_device, size, size / sizeof(uint32_t), false, m_testCubeIndices);
}

void RendererVK::DestroyShaders()
{
	m_testVertexShader.Destroy(&m_device);
	m_testFragmentShader.Destroy(&m_device);
}

void RendererVK::DestroyGraphicsPipeline()
{
	vkDestroyPipelineLayout(m_device.GetDevice(), m_testGraphicsPipelineLayout, nullptr);
	vkDestroyPipeline(m_device.GetDevice(), m_testGraphicsPipeline, nullptr);
}

void RendererVK::DestroyTestVertexAndTriangleBuffers()
{
	m_testVertexBuffer.Destroy(&m_device);
	m_testIndexBuffer.Destroy(&m_device);
}

void RendererVK::DestroyUniformBuffers()
{
	m_uboBuffers1.Destroy(&m_device);
	m_uboBuffers2.Destroy(&m_device);
}

void RendererVK::DestroyBackBuffer()
{
	for (size_t i = 0; i < m_backBuffer.m_frameBuffers.size(); ++i)
		m_backBuffer.m_frameBuffers[i].Destroy(&m_device);

	m_backBuffer.m_depthStencilBuffer.Destroy(&m_device);
}

void RendererVK::DestroyTextures()
{
	m_testTexture.Destroy(&m_device);
	m_testTexture2.Destroy(&m_device);
}

}