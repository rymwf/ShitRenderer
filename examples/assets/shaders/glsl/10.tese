#version 460
//the perpsective projection fliped y
#if VULKAN
#define VERTEX_ORDER cw
#else
#define VERTEX_ORDER ccw
#endif

layout(triangles, equal_spacing, VERTEX_ORDER) in;
//layout(isolines, equal_spacing, ccw) in;

layout(location=0) out vec4 color;

void main()
{
	float u=gl_TessCoord.x;
	float v=gl_TessCoord.y;
	float w=gl_TessCoord.z;

#if 1
	color=vec4(1);
	//triangles
	gl_Position=u*gl_in[0].gl_Position+
	v*gl_in[1].gl_Position+
	w*gl_in[2].gl_Position;
#endif
#if 0
	//isolines
	color=vec4(vec3(1)*(int(floor(u*gl_PatchVerticesIn))%2),1);
	gl_Position=mix(gl_in[0].gl_Position,gl_in[1].gl_Position,u);
#endif
}