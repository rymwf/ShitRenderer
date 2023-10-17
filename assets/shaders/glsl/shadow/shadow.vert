#version 460

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexcoord;

layout(location =0) out vec2 vs_texcoord;

layout(SET(0) binding=0) uniform UBOM
{
	mat4 M;
};

void main()
{
	gl_Position=M*vec4(inPosition,1);
	vs_texcoord=inTexcoord;
}

