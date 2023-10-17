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

layout (location = 0) out vec4 outColor;

layout(binding=0,r32ui)uniform readonly uimage2D counterImage;
layout(binding=1,rg32ui)uniform uimage2DArray outImages;

#define MAX_FRAGMENTS 16

uvec2 fragments[MAX_FRAGMENTS];

void sort_fragment_list(int fragCount)
{
	int j;
	uvec2 temp;
	float tempDepth;
	for(int i=1;i<fragCount;++i)
	{
		temp=fragments[i];
		tempDepth=uintBitsToFloat(temp.y);
		j=i;
		while(--j>=0&&uintBitsToFloat(fragments[j].y)<tempDepth)
		{
			fragments[j+1]=fragments[j];
		}
		fragments[j+1]=temp;
	}
}
vec4 calc_final_color(int fragCount)
{
	vec4 finalCol=vec4(0);
	for(int i=0;i<fragCount;++i)
	{
		vec4 newCol=unpackUnorm4x8(fragments[i].x);
		finalCol=mix(finalCol,newCol,newCol.a);
	}
	return finalCol;
}
void main() 
{
	ivec2 xy=ivec2(gl_FragCoord.xy);

	int fragCount=min(int(imageLoad(counterImage,xy).x),MAX_FRAGMENTS);
	for(int i=0;i<fragCount;++i)
		fragments[i]=imageLoad(outImages,ivec3(xy,i)).xy;
	sort_fragment_list(fragCount);
	outColor=calc_final_color(fragCount);
}