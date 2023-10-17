#version 460

#ifdef VULKAN
#define SET(x)set=x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#extension GL_EXT_scalar_block_layout:enable
#define PUSH_CONSTANT std430,binding=2
#endif

layout(location=0)in vec2 inTexcoord;
layout(location=0)out vec4 fragColor;
layout(binding=0)uniform sampler2D inTex;
layout(binding=1)buffer mySSBO
{
	int mode;
	int acesFilm;
	int kernelRadius;
	float kernel[];
};
#define LOD 4

vec3 f_filter(sampler2D tex,vec2 uv,int lod)
{
	ivec2 texDim=textureSize(tex,LOD);
	
	vec3 col=vec3(0);
	int w=kernelRadius*2+1;
	int l=w*w-1;
	for(int i=-kernelRadius;i<=kernelRadius;++i)
	{
		for(int j=-kernelRadius;j<=kernelRadius;++j)
		{
			col+=kernel[l-((i+kernelRadius)*w+j+kernelRadius)]*
			textureLod(tex,uv+vec2(i,j)/texDim,lod).rgb;
		}
	}
	return col;
}

vec3 ACESFilm(vec3 x)
{
	float a=2.51f;
	float b=.03f;
	float c=2.43f;
	float d=.59f;
	float e=.14f;
	return clamp((x*(a*x+b))/(x*(c*x+d)+e),0,1);
}

void main()
{
	vec3 col;
	switch(mode)
	{
		case 0:
		col=textureLod(inTex,inTexcoord,0).rgb;
		break;
		case 1:
		//only high light
		col=textureLod(inTex,inTexcoord,LOD).rgb;
		break;
		case 2:
		//blured high light
		col=f_filter(inTex,inTexcoord,LOD);
		break;
		case 3:
		//combined col
		col=textureLod(inTex,inTexcoord,0).rgb;
		col=min(col,vec3(1.));
		col+=f_filter(inTex,inTexcoord,LOD);
		break;
	}
	
	if(bool(acesFilm))
	col.rgb=ACESFilm(col.rgb);
	fragColor=vec4(col.rgb,1);
}