#version 450

#include "..\shaderCommon.h"

layout(set = 0, binding = UNIFORM_BUFFER_SLOT(0)) uniform UBO
{
	mat4x4 transform;
	vec4 testColor;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 texCoord;

void main()
{
	gl_Position = ubo.transform * vec4(inPosition, 1.0);
	texCoord = inTexCoord;
}
