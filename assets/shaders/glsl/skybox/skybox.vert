#version 460

#ifdef VULKAN
#define SET(x) set=x,
#define VertexIndex gl_VertexIndex
#else
#define SET(x)
#define VertexIndex gl_VertexID
#endif

const vec3 inPosition[]={
		{-1.0f, -1.0f, -1.0f},
		{-1.0f, 1.0f, -1.0f},
		{1.0f, -1.0f, -1.0f},
		{1.0f, 1.0f, -1.0f},

		{-1.0f, -1.0f, 1.0f},
		{-1.0f, 1.0f, 1.0f},
		{1.0f, -1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
};
//
const int inIndices[]={
	1,0,2,1,2,3,//-z
	6,4,5,6,5,7,//+z
	5,4,0,5,0,1,//-x
	3,2,6,3,6,7,//+x
	0,4,6,0,6,2,//-y
	1,7,5,1,3,7//+y
};

layout(SET(1) binding=0) buffer UBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
};
layout(location=0) out vec3 vs_position;

void main()
{
	mat4 V2=V;
	V2[3].xyz=vec3(0);
	gl_Position=P*V2*vec4(inPosition[inIndices[VertexIndex]],1);
	vs_position=inPosition[inIndices[VertexIndex]];
}
