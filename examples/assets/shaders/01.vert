#version 450 core
const vec2 inPos[3] = {{-0.5, -0.5}, {0.5, -0.5}, {-0.5, 0.5}};
#ifdef VULKAN
void main() { gl_Position = vec4(inPos[gl_VertexIndex], 0, 1); }
#else
void main() { gl_Position = vec4(inPos[gl_VertexID], 0, 1); }
#endif
