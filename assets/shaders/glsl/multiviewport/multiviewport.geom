#version 460
#extension GL_ARB_viewport_array : enable

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif

layout (triangles, invocations = 2) in;
layout (triangle_strip, max_vertices = 3) out;

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

layout(location =0) in vec3 normal[];
layout(location =1) in vec2 texcoord[];

layout(location =0) out vec3 gs_position;
layout(location =1) out vec3 gs_normal;
layout(location =2) out vec2 gs_texcoord;
layout(location =3) out vec3 gs_eyePos;

void main()
{
	for(int i=0;i<gl_in.length();++i)
	//for(int i=0;i<3;++i)
	{
		gl_Position=camera[gl_InvocationID].P*camera[gl_InvocationID].V*gl_in[i].gl_Position;

		gs_position=gl_in[i].gl_Position.xyz;
		gs_normal=normal[i];
		gs_texcoord=texcoord[i];
		gs_eyePos=camera[gl_InvocationID].eyePos;
		gl_ViewportIndex=gl_InvocationID;
		gl_PrimitiveID=gl_PrimitiveIDIn;
		EmitVertex();
	}
	EndPrimitive();
}

