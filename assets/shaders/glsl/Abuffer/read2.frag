#version 460

#ifdef VULKAN
#define SET(x)set=x,
#define SUBPASS_INPUT subpassInput
#define SUBPASS_INPUT_MS subpassInputMS
#define SUBPASS_LOAD(tex,b,c)subpassLoad(tex)
#define INPUT_ATTACHMENT_INDEX(x)input_attachment_index=x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#define SUBPASS_INPUT sampler2D
#define SUBPASS_INPUT_MS sampler2DMS
#define SUBPASS_LOAD(tex,xy,lod)texelFetch(tex,xy,lod)
#define INPUT_ATTACHMENT_INDEX(x)
#define PUSH_CONSTANT binding=0
#endif

layout(location=0)out vec4 outColor;

layout(binding=0)uniform usampler2D listHeaderImage;
layout(binding=1)uniform usamplerBuffer listBuffer;

#define MAX_FRAGMENTS 15

uvec4 fragments[MAX_FRAGMENTS];

int build_local_fragment_list()
{
	uint current=texelFetch(listHeaderImage,ivec2(gl_FragCoord.xy),0).x;
	int fragCount=0;
	uvec4 item;
	while(current!=0xFFFFFFFF&&fragCount<MAX_FRAGMENTS)
	{
		item=texelFetch(listBuffer,int(current));
		current=item.x;
		fragments[fragCount]=item;
		++fragCount;
	}
	return fragCount;
}
void sort_fragment_list(int fragCount)
{
	int j;
	uvec4 temp;
	float tempDepth;
	for(int i=1;i<fragCount;++i)
	{
		temp=fragments[i];
		tempDepth=uintBitsToFloat(temp.z);
		j=i;
		while(--j>=0&&uintBitsToFloat(fragments[j].z)<tempDepth)
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
		vec4 newCol=unpackUnorm4x8(fragments[i].y);
		finalCol=mix(finalCol,newCol,newCol.a);
	}
	return finalCol;
}
void main()
{
	ivec2 xy=ivec2(gl_FragCoord.xy);
	
	int fragCount=build_local_fragment_list();
	sort_fragment_list(fragCount);
	outColor=calc_final_color(fragCount);
}