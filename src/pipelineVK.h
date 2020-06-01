#pragma once

#include "commonVK.h"
#include "shaderVK.h"

namespace MBRF
{

class DeviceVK;
class FrameBufferVK;
class VertexFormatVK;

class PipelineVK
{
public:
	enum PipelineType
	{
		GRAPHICS,
		COMPUTE
	};

	PipelineVK(PipelineType type) : m_type(type) {};
	virtual ~PipelineVK() {};

	VkPipeline GetPipeline() const { return m_pipeline; };
	PipelineType GetType() const { return m_type; };

protected:
	PipelineType m_type;
	VkPipeline m_pipeline;
};

class GraphicsPipelineVK: public PipelineVK
{
public:
	GraphicsPipelineVK() : PipelineVK(GRAPHICS) {};

	// TODO: add all needed states
	bool Create(DeviceVK* device, VertexFormatVK* vertexFormat, FrameBufferVK* frameBuffer, std::vector<ShaderVK> shaders, bool backFaceCulling);
	void Destroy(DeviceVK* device);

	VkPipelineLayout GetLayout() const { return m_layout; };

private:
	VkPipelineLayout m_layout;
};

// TODO: implement compute PSO

}
