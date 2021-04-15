#version 460
const vec3 inPos[6] = {{0,0,0}, {10,0,0}, {0,0,0}, {0,10,0},{0,0,0}, {0,0,10}};

#ifdef VULKAN
#define VERTEXID gl_VertexIndex
#else
#define VERTEXID gl_VertexID
#endif

layout(location=0)out VS_OUT{
	vec3 pos;
};

layout(binding=12) uniform UBO_PV{
	mat4 PV;
};

void main() 
{ 
	gl_Position = PV*vec4(inPos[VERTEXID],1);
	pos=inPos[VERTEXID];
}

