#version 460

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif

layout(location=0) in vec3 inPosition;

layout(SET(0) binding=0) uniform UBOM
{
	mat4 M;
};
layout(SET(1) binding=0) buffer UBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
};

void main()
{
	gl_Position=P*V*M*vec4(inPosition,1);
	gl_PointSize=2;
}

