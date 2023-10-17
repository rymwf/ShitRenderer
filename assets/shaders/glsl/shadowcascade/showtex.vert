#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#define VertexIndex gl_VertexIndex
const vec2 inTexcoord[3] = {{0, 1}, {2, 1},{0, -1}};
#else
#define SET(x)
#define VertexIndex gl_VertexID
const vec2 inTexcoord[3] = {{0, 0}, {2, 0},{0, 2}};
#endif

const vec2 inPos[3] = {{-1, -1}, {3, -1},{-1, 3}};

layout(location =0) out vec2 vs_texcoord;

void main()
{
	gl_Position = vec4(inPos[VertexIndex], 0, 1); 
	vs_texcoord=inTexcoord[VertexIndex];
}

