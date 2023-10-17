#version 460

#ifdef VULKAN
#define SET(x) set=x,
#define SUBPASS_INPUT subpassInput 
#define SUBPASS_INPUT_MS subpassInputMS 
#define SUBPASS_LOAD(tex,b,c) subpassLoad(tex)
#define INPUT_ATTACHMENT_INDEX(x) input_attachment_index = x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#define SUBPASS_INPUT sampler2D 
#define SUBPASS_INPUT_MS sampler2DMS
#define SUBPASS_LOAD(tex,xy,lod) texelFetch(tex,xy,lod)
#define INPUT_ATTACHMENT_INDEX(x)
#define PUSH_CONSTANT binding=0
#endif

layout (INPUT_ATTACHMENT_INDEX(0) binding = 0) uniform SUBPASS_INPUT inputCol;

layout (location = 0) out vec4 outColor;

void main() 
{
	ivec2 xy=ivec2(gl_FragCoord.xy);
	outColor=SUBPASS_LOAD(inputCol,xy,0);
}