#ifdef VULKAN
#define SET(x) ,set=x
#else
#version 460 
#define SET(x)
#endif

layout(location=0) vec4 outPos;
layout(location=1) vec4 outAlbedo;
layout(location=2) vec4 outNormal;
layout(location=3) vec4 outMetallicRoughness;
layout(location=4) vec4 outOcclution;
layout(location=5) vec4 outEmission;
layout(location=6) vec4 outTransparency;


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

layout(binding=14 SET(2)) uniform UBOMaterial{
	vec3 emissiveFactor;
	float alphaCutoff;
	vec4 baseColorFactor;	
	float metallic;
	float roughness;
	float normalScale;
}uboMaterial;

layout(binding=12 SET(0)) uniform UBOFrame{
	mat4 PV;
	vec3 eyePosition;
	Light light;
	vec3 ambientColor;
};

mat3 TBN(vec3 N)
{
	vec3 up=vec3(0,1,0);
	vec3 T=cross(up,N);
	vec3 B=cross(N,T);
	return mat3(T,B,N);
}

void main()
{
	vec4 outPos=vec4(fs_in.pos,1);
	vec4 outAlbedo=pow(texture(texAlbedo,fs_in.texCoord),vec4(2.2))*uboMaterial.baseColorFactor*fs_in.colorFactor;
	vec4 outNormal=vec4(normalize(TBN(fs_in.normal)*((texture(texNormal,fs_in.texCoord).rgb*2-1)*vec3(uboMaterial.normalScale,uboMaterial.normalScale,1.))),1);
vec4 outMetallicRoughness=texture(texMetallicRoughness,fs_in.texCoord);;
vec4 outOcclution=texture(texOcclusion,fs_in.texCoord);
vec4 outEmission;
vec4 outTransparency;
	vec3 emissiveColor=pow(texture(texEmission,fs_in.texCoord).rgb*uboMaterial.emissiveFactor,vec3(2.2));
	vec3 metallicRoughness=

}