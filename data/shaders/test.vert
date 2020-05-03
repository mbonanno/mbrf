#version 450

vec2 triangleVerts[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
	);

vec4 vertColors[3] = vec4[](
	vec4(1.0, 0.0, 0.0, 1.0),
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(0.0, 0.0, 1.0, 1.0)
	);

layout(set = 0, binding = 0) uniform UBO
{
	vec4 testColor;
}ubo;

layout(push_constant) uniform PushConsts
{
	vec4 pushTest;
}pushConsts;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main()
{
	//gl_Position = vec4(triangleVerts[gl_VertexIndex], 0.0, 1.0);
	//outColor = vertColors[gl_VertexIndex];

	gl_Position = vec4(inPosition, 0.0, 1.0);
	outColor = ubo.testColor; //pushConsts.pushTest; //inColor
}
