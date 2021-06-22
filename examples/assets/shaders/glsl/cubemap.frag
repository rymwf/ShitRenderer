
#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif

layout(location = 0) out vec4 outColor;

layout(location = 0) in VS_OUT { 
	vec3 texCoord;
}fs_in;

layout(binding=0 SET(1)) uniform samplerCube texCube;

layout(binding=0 SET(2)) uniform UBOAA
{
	vec4 tintColor;
	float exposure;
};

void main()
{
	//outColor=texture(texCube,fs_in.texCoord);
	outColor=vec4(texture(texCube,fs_in.texCoord).rgb*exposure,1)*(tintColor*2);
	//outColor=vec4(fs_in.texCoord,1);
}