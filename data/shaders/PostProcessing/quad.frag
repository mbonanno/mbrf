#version 450

#include "..\shaderCommon.h"

layout(set = 0, binding = TEXTURE_SLOT(0)) uniform sampler2D offscreenTex;
layout(set = 0, binding = TEXTURE_SLOT(1)) uniform sampler2D vignetteTex;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = texture(offscreenTex, inTexCoord).rgba + texture(vignetteTex, inTexCoord).rgba * 0.05;
}
