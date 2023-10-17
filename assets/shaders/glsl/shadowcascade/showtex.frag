#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#define PUSH_CONSTANT std430, binding=1
#extension GL_EXT_scalar_block_layout: enable
#endif

layout(early_fragment_tests)in;

layout(location=0) out vec4 outColor;
layout(location =0) in vec2 fs_texcoord;

layout(binding=0) uniform sampler2DArray tex;

layout(PUSH_CONSTANT) uniform UBOBuffer
{
	int texLayer;
};

void main()
{
	//float 
	float depth=texture(tex,vec3(fs_texcoord,texLayer)).r;
	outColor= vec4(vec3(depth),1);
}