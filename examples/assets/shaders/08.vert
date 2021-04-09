#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;
layout(location = 11) in vec4 inInstanceColorFactor;
layout(location = 12) in mat4 inInstanceMatrix;

layout(location = 0) out VS_OUT { 
	vec4 colorFactor;
	vec2 texCoord;
	vec3 pos;
	vec3 normal;
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

void main() 
{
	mat4 tempMat=inInstanceMatrix*M;
	gl_Position = PV*tempMat*vec4(inPos, 1);
	vs_out.colorFactor = inInstanceColorFactor;
	vs_out.texCoord= inTexCoord;
	vs_out.pos= mat3(tempMat)*inPos;
	vs_out.normal= mat3(tempMat)*inNormal;
}
