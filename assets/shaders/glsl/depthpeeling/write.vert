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

struct VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
	float depth;
};
layout(location =0) out VS_OUT vs_out;

void main()
{
	//gl_Position=vec4(inPosition,1);
	gl_Position=P*V*M*vec4(inPosition,1);
	vs_out.depth=gl_Position.z/gl_Position.w;
	vs_out.position=mat4x3(M)*vec4(inPosition,1);
	vs_out.normal=normalize(mat3(M)*inNormal);
	vs_out.texcoord=inTexcoord;
}

