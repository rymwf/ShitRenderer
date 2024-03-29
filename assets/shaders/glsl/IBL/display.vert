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
layout(SET(1) binding=0) buffer UBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
};

layout(location =0) out vec3 vs_position;
layout(location =1) out vec3 vs_normal;
layout(location =2) out vec2 vs_texcoord;

void main()
{
	//gl_Position=vec4(inPosition,1);
	gl_Position=P*V*M*vec4(inPosition,1);
	vs_position=mat4x3(M)*vec4(inPosition,1);
	vs_normal=normalize(mat3(M)*inNormal);
	vs_texcoord=inTexcoord;
}

