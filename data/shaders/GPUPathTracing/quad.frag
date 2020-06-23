#version 450

#include "..\shaderCommon.h"

layout(set = 0, binding = TEXTURE_SLOT(0)) uniform sampler2D offscreenTex;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 OutColor;

// ACES tone mapping curve fit to go from HDR to LDR
//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0f, 1.0f);
}

void main()
{
	vec3 color = texture(offscreenTex, inTexCoord).rgb;

	// apply exposure (how long the shutter is open)
    color *= 0.5;
    
    color = ACESFilm(color);
    
	// no need of color space conversion as the swapchain RThas an srgb format
    //color = LinearToSRGB(color);

	OutColor = vec4(color, 1.0);
}
