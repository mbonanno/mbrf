#version 450

#include "..\shaderCommon.h"

layout(location = 0) in vec4 inColor;

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = inColor;
}
