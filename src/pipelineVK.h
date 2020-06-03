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

enum CullMode
{
	CULL_MODE_BACK,
	CULL_MODE_FRONT,
	CULL_MODE_NONE,
	NUM_CULL_MODES
};

struct GraphicsPipelineDesc
{
	VertexFormatVK* m_vertexFormat = nullptr;
	FrameBufferVK* m_frameBuffer = nullptr;
	std::vector<ShaderVK> m_shaders;
	CullMode m_cullMode = CULL_MODE_NONE;
};

class GraphicsPipelineVK: public PipelineVK
{
public:
	GraphicsPipelineVK() : PipelineVK(GRAPHICS) {};

	// TODO: add all needed states
	bool Create(DeviceVK* device, const GraphicsPipelineDesc &desc);
	void Destroy(DeviceVK* device);

	VkPipelineLayout GetLayout() const { return m_layout; };

private:
	VkPipelineLayout m_layout;
};

// TODO: implement compute PSO

}
