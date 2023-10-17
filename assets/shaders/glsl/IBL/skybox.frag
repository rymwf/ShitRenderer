#version 460
#extension GL_EXT_scalar_block_layout :enable

#define PI 3.141592653

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif
layout(early_fragment_tests)in;

layout(location=0) out vec4 outColor;

layout(location =0) in vec3 fs_position;

layout(SET(0) binding=0) uniform samplerCube skybox_tex;
layout(SET(0) binding=2) uniform UBOOther
{
	int enableACESFilmic;
};

vec3 ACESFilm(vec3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x*(a*x+b))/(x*(c*x+d)+e),0,1);
}

void main()
{
	vec3 uv= normalize(fs_position);
	outColor=vec4(textureLod(skybox_tex,uv,0).rgb,1);
	if(bool(enableACESFilmic))
		outColor.rgb=ACESFilm(outColor.rgb);
}