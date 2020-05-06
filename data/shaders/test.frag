#version 450

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;

layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 OutColor;

void main()
{
	OutColor = texture(texSampler, inTexCoord).rgba; //inColor;
}
