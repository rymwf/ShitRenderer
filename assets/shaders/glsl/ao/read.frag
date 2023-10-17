#version 460

#ifdef VULKAN
#define SET(x) set=x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#define PUSH_CONSTANT binding=1
#endif

layout (location = 0) in vec2 inTexcoord;
layout (location = 0) out vec4 outColor;
layout (binding= 0) uniform sampler2D inTex;

void main() 
{
	outColor=texture(inTex,inTexcoord);
	//outColor=vec4(inTexcoord,0,1);
}