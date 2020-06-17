#version 450

#include "..\shaderCommon.h"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;

void main()
{
	gl_Position = vec4(inPosition, 1.0);
	outTexCoord = inTexCoord;
}
