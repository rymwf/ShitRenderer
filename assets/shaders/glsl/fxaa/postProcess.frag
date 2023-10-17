#version 460

#ifdef VULKAN
#define PUSH_CONSTANT push_constant
#else
#extension GL_EXT_scalar_block_layout:enable
#define PUSH_CONSTANT std430,binding=1
#endif

layout(location=0)in vec2 inTexcoord;
layout(location=0)out vec4 outColor;

layout(binding=0)uniform sampler2D inTex;

layout(PUSH_CONSTANT)uniform uPushConstant
{
	int fxaa;
	float fxaaThreashold;
	float fxaaThreasholdMin;
	float fxaaSharpness;
	int highlightArea;
};

#define FXAA_SEARCH_STEPS 1
#define FXAA_SEARCH_ACCELERATION 1

//#define FXAA_EDGE_THRESHOLD_MIN 0.01
//#define FXAA_EDGE_THRESHOLD .1

void main()
{
	//local contrast check
	vec4 rgbM=texture(inTex,inTexcoord);
	if(fxaa==0)
	{
		outColor=rgbM;
		return;
	}
	float lumaM=rgbM.g;
	
	ivec2 texDim=textureSize(inTex,0);
	vec2 duv=1./texDim;
	
	float lumaNW=texture(inTex,inTexcoord+duv*vec2(-1,-1)).g;
	float lumaNE=texture(inTex,inTexcoord+duv*vec2(1,-1)).g;
	float lumaSW=texture(inTex,inTexcoord+duv*vec2(-1,1)).g;
	float lumaSE=texture(inTex,inTexcoord+duv*vec2(1,1)).g;
	lumaNE+=1./384.;
	
	float lumaMax=max(lumaM,max(max(lumaNW,lumaNE),max(lumaSW,lumaSE)));
	float lumaMin=min(lumaM,min(min(lumaNW,lumaNE),min(lumaSW,lumaSE)));
	if((lumaMax-lumaMin)<max(fxaaThreasholdMin,lumaMax*fxaaThreashold))
	{
		outColor=rgbM;
		return;
	}
	//outColor=vec4(1,0,0,1);
	//return;

	float lumaSW_NE=lumaSW-lumaNE;
	float lumaSE_NW=lumaSE-lumaNW;
	vec2 dir1=normalize(vec2(lumaSW_NE+lumaSE_NW,lumaSW_NE-lumaSE_NW));
	vec4 rgbyN1=texture(inTex,inTexcoord-dir1*duv);
	vec4 rgbyP1=texture(inTex,inTexcoord+dir1*duv);
	
	float dirAbsMinTimesC=min(abs(dir1.x),abs(dir1.y))*fxaaSharpness;
	vec2 dir2=clamp(dir1/dirAbsMinTimesC,-2.,2.);
	vec4 rgbyN2=texture(inTex,inTexcoord-dir2*duv);
	vec4 rgbyP2=texture(inTex,inTexcoord+dir2*duv);
	
	vec4 rgbyA=rgbyN1+rgbyP1;
	vec4 rgbyB=((rgbyN2+rgbyP2)*.25)+(rgbyA*.25);
	
	bool twoTap=(rgbyB.y<lumaMin)||(rgbyB.y>lumaMax);
	
	if(twoTap)
	rgbyB.xyz=rgbyA.xyz*.5;
	outColor=rgbyB;
	if(bool(highlightArea))
		outColor.rgb*=vec3(1,0,0);
}