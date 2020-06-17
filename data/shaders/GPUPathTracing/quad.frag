#version 450

#include "..\shaderCommon.h"

layout(set = 0, binding = TEXTURE_SLOT(0)) uniform sampler2D offscreenTex;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 OutColor;

vec3 LessThan(vec3 f, float value)
{
    return vec3(
        (f.x < value) ? 1.0f : 0.0f,
        (f.y < value) ? 1.0f : 0.0f,
        (f.z < value) ? 1.0f : 0.0f);
}

vec3 LinearToSRGB(vec3 rgb)
{
    rgb = clamp(rgb, 0.0f, 1.0f);
     
    return mix(
        pow(rgb, vec3(1.0f / 2.4f)) * 1.055f - 0.055f,
        rgb * 12.92f,
        LessThan(rgb, 0.0031308f)
    );
}
 
vec3 SRGBToLinear(vec3 rgb)
{
    rgb = clamp(rgb, 0.0f, 1.0f);
     
    return mix(
        pow(((rgb + 0.055f) / 1.055f), vec3(2.4f)),
        rgb / 12.92f,
        LessThan(rgb, 0.04045f)
    );
}

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
	vec4 color = texture(offscreenTex, inTexCoord).rgba;

	// apply exposure (how long the shutter is open)
    color.rgb *= 1;
    
    color.rgb = ACESFilm(color.rgb);
    
    //color.rgb = LinearToSRGB(color.rgb);

	OutColor = color; // + texture(vignetteTex, inTexCoord).rgba * 0.05;
}
