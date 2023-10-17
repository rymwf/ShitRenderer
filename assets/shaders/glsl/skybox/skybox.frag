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
void main()
{
	vec3 uv= normalize(fs_position);
	outColor=vec4(textureLod(skybox_tex,uv,0).rgb,1);
}