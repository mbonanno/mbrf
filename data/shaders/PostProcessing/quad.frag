#version 450

#include "..\shaderCommon.h"

layout(set = 0, binding = UNIFORM_BUFFER_SLOT(0)) uniform UBO
{
	float nearPlane;
	float farPlane;
} ubo;

layout(set = 0, binding = TEXTURE_SLOT(0)) uniform sampler2D offscreenTex;
layout(set = 0, binding = TEXTURE_SLOT(1)) uniform sampler2D depthTex;
layout(set = 0, binding = TEXTURE_SLOT(2)) uniform sampler2D vignetteTex;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 OutColor;

float LinearizeDepth(float depth)
{
  float n = ubo.nearPlane; // camera z near
  float f = ubo.farPlane; // camera z far
  float z = depth;
  return (2.0 * n) / (f + n - z * (f - n));	
}

void main()
{
	float depth = LinearizeDepth(texture(depthTex, inTexCoord).r);

	OutColor = texture(offscreenTex, inTexCoord).rgba + texture(vignetteTex, inTexCoord).rgba * 0.05;
}
