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
}
vs_out;

layout(binding=12 SET(1)) uniform UBOM{
	mat4 M;
};
layout(binding=13 SET(0)) uniform UBOFrame{
	mat4 PV;
};

void main() 
{
  gl_Position = PV*inInstanceMatrix*M*vec4(inPos, 1);
  //gl_Position = vec4(inPos, 1);
  vs_out.colorFactor = inInstanceColorFactor;
  vs_out.texCoord= inTexCoord;
}
