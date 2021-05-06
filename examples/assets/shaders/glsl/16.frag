#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif

layout(location = 0) out vec4 outColor;

layout(binding=2 SET(2)) uniform sampler2D texMetallicRoughness;
layout(binding=1 SET(2)) uniform sampler2D texNormal;

layout(binding=12 SET(0)) uniform UBOFrame{
	mat4 PV;
	vec3 eyePosition;
};
layout(binding=14 SET(2)) uniform UBOMaterial{
	vec3 emissiveFactor;
	float alphaCutoff;
	vec4 baseColorFactor;	
	float metallic;
	float roughness;
	float normalScale;
}uboMaterial;

layout(location = 0) in VS_OUT { 
	vec3 pos;
	vec3 normal;
	vec2 texCoord;
}fs_in;

layout(binding=6 SET(4)) uniform samplerCube texCube;

layout(constant_id=3) const int maxRoughnessLevel=7;

mat3 surfaceTBN(vec3 N)
{
	vec3 up=abs(N.z)>0.999?vec3(1,0,0):vec3(0,0,1);
	vec3 T=cross(up,N);
	vec3 B=cross(N,T);
	return mat3(T,B,N);
}

void main()
{
	vec3 N=normalize(surfaceTBN(fs_in.normal)*((texture(texNormal,fs_in.texCoord).rgb*2-1)*vec3(uboMaterial.normalScale,uboMaterial.normalScale,1.)));
	vec3 V=normalize(eyePosition-fs_in.pos);
	vec3 R=reflect(-V,N);

	vec3 metallicRoughness=texture(texMetallicRoughness,fs_in.texCoord).rgb;
	float roughness=metallicRoughness.y*uboMaterial.roughness;

	outColor=textureLod(texCube,R,roughness*maxRoughnessLevel);
	//outColor=vec4(1);
}