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
		PIPELINE_TYPE_GRAPHICS,
		PIPELINE_TYPE_COMPUTE
	};

	PipelineVK(PipelineType type) : m_type(type) {};
	virtual ~PipelineVK() {};

	VkPipeline GetPipeline() const { return m_pipeline; };
	PipelineType GetType() const { return m_type; };

	VkPipelineBindPoint GetBindPoint();

	virtual VkPipelineLayout GetLayout() const = 0;

protected:
	void Destroy(DeviceVK* device);

protected:
	PipelineType m_type;
	VkPipeline m_pipeline = VK_NULL_HANDLE;
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

// TODO: for now using a different layout per pipeline, even though they are actually using the same layout, 
// but might be handy in the future if we want to differentiate?

class GraphicsPipelineVK: public PipelineVK
{
public:
	GraphicsPipelineVK() : PipelineVK(PIPELINE_TYPE_GRAPHICS) {};

	// TODO: add all needed states
	bool Create(DeviceVK* device, const GraphicsPipelineDesc &desc);
	void Destroy(DeviceVK* device);

	VkPipelineLayout GetLayout() const { return m_layout; };

private:
	VkPipelineLayout m_layout = VK_NULL_HANDLE;
};

class ComputePipelineVK : public PipelineVK
{
public:
	ComputePipelineVK() : PipelineVK(PIPELINE_TYPE_COMPUTE) {};

	bool Create(DeviceVK* device, ShaderVK* computeShader);
	void Destroy(DeviceVK* device);

	VkPipelineLayout GetLayout() const { return m_layout; };

private:
	VkPipelineLayout m_layout = VK_NULL_HANDLE;
};

}
