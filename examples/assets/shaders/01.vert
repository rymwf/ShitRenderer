#version 450 core
const vec2 inPos[4] = {{-0.5, -0.5}, {-0.5, 0.5}, {0.5, -0.5}, {0.5,0.5}};
const vec4 color[4]={{1,0,0,1},{0,1,0,1},{0,0,1,1},{1,1,1,1}};

struct VS_OUT{
	vec4 color;
};

layout(location=0) out VS_OUT vs_out;

#ifdef VULKAN
void main() { 
	gl_Position = vec4(inPos[gl_VertexIndex], 0, 1); 
	vs_out.color=color[gl_VertexIndex];
}
#else
void main() { 
	gl_Position = vec4(inPos[gl_VertexID], 0, 1);
	vs_out.color=color[gl_VertexID];
 }
#endif
