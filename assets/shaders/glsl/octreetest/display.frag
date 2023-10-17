#version 460
#extension GL_EXT_scalar_block_layout :enable

#ifdef VULKAN
#define SET(x) set=x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#define PUSH_CONSTANT std430,binding=1
#endif
//layout(early_fragment_tests)in;

layout(location=0) out vec4 outColor;

layout(SET(1) binding=0) buffer UBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
};
layout(PUSH_CONSTANT) uniform uPushConstant
{
	vec4 col;
}uPc;

void main()
{
	outColor=uPc.col;
}