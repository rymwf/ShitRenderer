#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif
layout(location=0)in vec4 inColor;
layout(location = 0) out vec4 outColor;

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

layout(constant_id=0) const int jointNum=0;
layout(binding=15 SET(3)) uniform UBOJointMatrix
{
	mat4 jointMatrices[jointNum+1];
};

void main() 
{
	//outColor=vec4(1,1,1,1);
	outColor=inColor;
}
