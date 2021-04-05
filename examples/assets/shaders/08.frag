#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif

layout(location = 0) out vec4 outColor;
layout(location = 0) in VS_OUT { 
	vec4 colorFactor; 
	vec2 texCoord;
}fs_in;

layout(binding=0 SET(2)) uniform sampler2D texAlbedo;
layout(binding=1 SET(2)) uniform sampler2D texNormal;
layout(binding=2 SET(2)) uniform sampler2D texMetallicRoughness;
layout(binding=3 SET(2)) uniform sampler2D texOcclusion;
layout(binding=4 SET(2)) uniform sampler2D texEmission;
layout(binding=5 SET(2)) uniform sampler2D texTransparency;

layout(binding=14 SET(2)) uniform ubo_material{
	vec3 emissiveFactor;
	float alphaCutoff;
	vec4 baseColorFactor;	
	float metallic;
	float roughness;
};

void main() 
{
//   outColor = vec4(1,0,0,1);
//   outColor = fs_in.colorFactor;
	outColor = vec4(texture(texAlbedo,fs_in.texCoord).rgb,1.);
	//outColor = vec4(texture(texNormal,fs_in.texCoord).rgb*0.5+0.5,1);
	//outColor = texture(texAlbedo,fs_in.texCoord);
}
