
#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#define VERTEXID gl_VertexIndex
#else
#define SET(x)
#define VERTEXID gl_VertexID
#endif

vec3 vertices[36]={
	{1,-1,-1},{1,-1,1},{1,1,-1},{1,1,-1},{1,-1,1},{1,1,1},	//+x
	{-1,-1,1},{-1,-1,-1},{-1,1,1},{-1,1,1},{-1,-1,-1},{-1,1,-1},//-x

	{-1,1,-1},{1,1,-1},{-1,1,1},{-1,1,1},{1,1,-1},{1,1,1},	//+y
	{1,-1,-1},{-1,-1,-1},{1,-1,1},{1,-1,1},{-1,-1,-1},{-1,-1,1},	//-y

	{1,-1,1},{-1,-1,1},{1,1,1},{1,1,1},{-1,-1,1},{-1,1,1},	//+z
	{-1,-1,-1},{1,-1,-1},{-1,1,-1},{-1,1,-1},{1,-1,-1},{1,1,-1},	//-z
};

layout(location = 0) out VS_OUT { 
	vec3 texCoord;
}vs_out;

layout(binding=12 SET(0)) uniform UBOFrame{
	mat4 PV;
	vec3 eyePos;
};

void main()
{
	gl_Position = (PV*vec4(vertices[VERTEXID]+eyePos, 1)).xyww;
	vs_out.texCoord= vertices[VERTEXID];
}