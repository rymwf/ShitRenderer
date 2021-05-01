#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif

layout(location = 0) out vec4 outColor;

layout(location = 0) in VS_OUT { 
	vec3 pos;
	vec3 normal;
	vec2 texCoord;
}fs_in;

layout(binding=6 SET(4)) uniform samplerCube texCube;

void main()
{
	outColor=texture(texCube,fs_in.normal);
	//outColor=vec4(1);
}