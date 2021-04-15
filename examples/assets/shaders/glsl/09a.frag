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
	vec3 pos;
	vec3 normal;
}fs_in;

layout(binding=0 SET(2)) uniform sampler2D texAlbedo;
layout(binding=1 SET(2)) uniform sampler2D texNormal;
layout(binding=2 SET(2)) uniform sampler2D texMetallicRoughness;
layout(binding=3 SET(2)) uniform sampler2D texOcclusion;
layout(binding=4 SET(2)) uniform sampler2D texEmission;
layout(binding=5 SET(2)) uniform sampler2D texTransparency;

layout(binding=14 SET(2)) uniform uboMaterial{
	vec3 emissiveFactor;
	float alphaCutoff;
	vec4 baseColorFactor;	
	float metallic;
	float roughness;
	int normalFactor;
};

struct Light{
	vec3 pos;
	vec4 color;
	vec4 intensity;
	vec2 lim_r; //the attenuation distance limit,r.x: min dist(sphere light radius), r.y: max dist
	vec3 direction;
	vec3 tube_p0;
	vec3 tube_p1;
};
layout(binding=12 SET(0)) uniform UBOFrame{
	mat4 PV;
	Light light;
	vec3 ambientColor;
};

void main() 
{
	vec3 albedo=pow(texture(texAlbedo,fs_in.texCoord).rgb*baseColorFactor.rgb,vec3(2.2));
	vec3 emissiveColor=pow(texture(texEmission,fs_in.texCoord).rgb*emissiveFactor,vec3(2.2));

	vec3 L=normalize(light.pos-fs_in.pos);
	vec3 N=normalize(normalFactor*(texture(texNormal,fs_in.texCoord).rgb*2-1)+normalize(fs_in.normal));

	//lambertian
	outColor=vec4(albedo*(light.color.rgb*light.intensity.rgb*max(dot(L,N),0)+ambientColor)+emissiveColor,1);



}
