#version 460

#ifdef VULKAN
#define VertexIndex gl_VertexIndex
#else
#define VertexIndex gl_VertexID
#endif

layout(location=0) out vec2 outTexcoord;

const vec2 inPos[3] = {{-1, -1}, {3, -1},{-1, 3}};
const vec2 inTexcoord[3] = {{0, 1}, {2, 1},{0, -1}};

void main() { 
	gl_Position = vec4(inPos[VertexIndex], 0, 1); 
	outTexcoord=inTexcoord[VertexIndex];
}
