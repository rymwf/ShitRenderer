#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif

layout(location = 0) out vec4 outColor;
layout(location = 0) in VS_OUT { 
	vec4 color;
}fs_in;

void main() 
{
   outColor = fs_in.color;
}
