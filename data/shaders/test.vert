#version 450

vec3 triangleVerts[3] = vec3[](
    vec3(0.0, -0.5, 0.0),
    vec3(0.5, 0.5, 0.0),
    vec3(-0.5, 0.5, 0.0)
	);

vec4 vertColors[3] = vec4[](
	vec4(1.0, 0.0, 0.0, 1.0),
	vec4(0.0, 1.0, 0.0, 1.0),
	vec4(0.0, 0.0, 1.0, 1.0)
	);

layout(set = 0, binding = 0) uniform UBO
{
	mat4x4 test;
	vec4 testColor;
}ubo;

layout(push_constant) uniform PushConsts
{
	vec4 pushTest;
}pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main()
{
	//gl_Position = vec4(triangleVerts[gl_VertexIndex], 1.0);
	//outColor = vertColors[gl_VertexIndex];

	gl_Position = ubo.test * vec4(inPosition, 1.0);
	outColor = inColor; //ubo.testColor; //pushConsts.pushTest; //inColor
}
