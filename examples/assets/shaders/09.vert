#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#define PUSH_CONSTANT binding=15
#endif

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;
layout(location = 11) in vec4 inInstanceColorFactor;
layout(location = 12) in mat4 inInstanceMatrix;

layout(location = 0) out VS_OUT { 
	vec4 color;
}
vs_out;

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

layout(binding=13 SET(1)) uniform UBONode{
	mat4 M;
};

layout(binding=14 SET(2)) uniform uboMaterial{
	vec3 emissiveFactor;
	float alphaCutoff;
	vec4 baseColorFactor;	
	float metallic;
	float roughness;
};

void main() 
{
	vec3 albedo=baseColorFactor.rgb;
	vec3 L=normalize(light.pos-mat3(M)*inPos);
	vs_out.color=vec4(albedo*(light.color.rgb*max(dot(L,mat3(M)*inNormal),0)+ambientColor)+emissiveFactor,1.);
	gl_Position = PV*M*vec4(inPos, 1);
}
