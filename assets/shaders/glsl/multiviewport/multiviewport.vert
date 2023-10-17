#version 460

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec2 inTexcoord;

layout(SET(0) binding=0) uniform UBOM
{
	mat4 M;
};
struct Camera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
};
layout(SET(1) binding=2) uniform UBOCamera
{
	Camera camera[2];
};

layout(location =0) out vec3 normal;
layout(location =1) out vec2 texcoord;

void main()
{
	gl_Position=M*vec4(inPosition,1);
	normal=mat3(M)*inNormal;
	texcoord=inTexcoord;
}

