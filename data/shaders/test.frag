#version 450

#include "shaderCommon.h"

layout(set = 0, binding = TEXTURE_SLOT(0)) uniform sampler2D texSampler;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = texture(texSampler, inTexCoord).rgba; //inColor;
}
