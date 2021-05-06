#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif

layout(location = 0) out vec4 outColor;

layout(binding=2 SET(2)) uniform sampler2D texMetallicRoughness;

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

void main()
{
	vec3 metallicRoughness=texture(texMetallicRoughness,fs_in.texCoord).rgb;
	float roughness=metallicRoughness.y*uboMaterial.roughness;

	outColor=textureLod(texCube,fs_in.normal,roughness*maxRoughnessLevel);
	//outColor=vec4(1);
}