#version 460

#ifdef VULKAN
#extension GL_EXT_multiview : enable
#define VIEWID gl_ViewIndex
#define SET(x) set=x,
#else
#extension GL_OVR_multiview : enable
#define VIEWID gl_ViewID_OVR
#define SET(x)
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

struct VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
};
layout(location=0) out VS_OUT vs_out;

void main()
{
	gl_Position=camera[VIEWID].P*camera[VIEWID].V*M*vec4(inPosition,1);
	vs_out.position=mat4x3(M)*vec4(inPosition,1);
	vs_out.normal=normalize(mat3(M)*inNormal);
	vs_out.texcoord=inTexcoord;
}

