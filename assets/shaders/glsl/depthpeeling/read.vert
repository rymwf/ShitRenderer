#version 460

const vec2 inPos[3] = {{-1, -1}, {3, -1},{-1, 3}};

#ifdef VULKAN
#define VertexIndex gl_VertexIndex
#else
#define VertexIndex gl_VertexID
#endif

void main() { 
	gl_Position = vec4(inPos[VertexIndex], 0, 1); 
}