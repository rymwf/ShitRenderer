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

struct Light{
	vec3 pos;
	vec4 intensity;
	vec3 direction;
	vec3 points[4];
	float lim_r_min;
	float lim_r_max;
	float thetaPenumbra;
	float thetaUmbra;
};
layout(binding=12 SET(0)) uniform UBOFrame{
	mat4 PV;
	vec3 eyePosition;
	Light light;
	vec3 ambientColor;
};

layout(binding=6 SET(4)) uniform samplerCube texCube;

void main()
{
	vec3 V=normalize(eyePosition-fs_in.pos);
	vec3 R=reflect(-V,fs_in.normal);

	outColor=texture(texCube,R);
	//outColor=vec4(1);
}