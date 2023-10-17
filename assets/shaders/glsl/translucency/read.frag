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

#define EPSILON 1e-4

layout(location=0)out vec4 outColor;

layout(binding=0,r32ui)uniform readonly uimage2D counterImage;
layout(binding=1,rgba32ui)uniform uimage2DArray outImages;

layout(binding=8)uniform samplerCube prefilteredEnvTex;
layout(binding=9)uniform sampler2D envBrdfTex;

layout(PUSH_CONSTANT)uniform uPushConstant
{
	vec3 extinction;
	float extinctionFactor;
	float ior;
	float roughness;
	int fresnelEffect;
	int enableReflection;
}uPc;

layout(SET(1)binding=0)buffer UBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
}uboCamera;

layout(SET(3)binding=1)buffer UBOLight
{
	mat4 transformMatrix;
	vec3 color;
	int lightType;// 1 directional, 2 sphere, 3 spot, 4 tube, disable when <0
	vec3 radius;
	float rmax;
	float radiance;
	float cosThetaU;
	float cosThetaP;
	int vertexNum;
	vec3 vertices[4];
}uboLight;

#define MAX_FRAGMENTS 16
#define ROUGHNESS_1_LEVEL 7

uvec4 fragments[MAX_FRAGMENTS];

//descent order
void sort_fragment_list(int fragCount)
{
	int j;
	uvec4 temp;
	float tempDepth;
	for(int i=1;i<fragCount;++i)
	{
		temp=fragments[i];
		tempDepth=uintBitsToFloat(temp.y);
		j=i;
		while(--j>=0&&uintBitsToFloat(fragments[j].y)>tempDepth)
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
vec3 fresnelSchlick(float NdotL,vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - clamp(NdotL,0,1), 5.0);
	//return F0+(1.-F0)*exp2((-5.55473*NdotL-6.98316)*NdotL);
}
void main()
{
	ivec2 xy=ivec2(gl_FragCoord.xy);
	ivec2 imageDim=imageSize(counterImage);
	
	int fragCount=min(int(imageLoad(counterImage,xy).x),MAX_FRAGMENTS);
	for(int i=0;i<fragCount;++i)
		fragments[i]=imageLoad(outImages,ivec3(xy,i));
	sort_fragment_list(fragCount);
	
	vec3 fragPostionsV[MAX_FRAGMENTS];
	vec3 fragNormals[MAX_FRAGMENTS];
	vec3 fragNormalsV[MAX_FRAGMENTS];
	//float fragDistanceAlongNormal[MAX_FRAGMENTS];
	mat4 Pinv=inverse(uboCamera.P);
	
	vec3 p=vec3(vec2(gl_FragCoord.xy)/imageDim,0);
	p.xy*=2;
	p.xy-=1;
	p.y=-p.y;
	for(int i=0;i<fragCount;++i)
	{
		p.z=uintBitsToFloat(fragments[i].y);
		vec4 a=Pinv*vec4(p,1);
		a.xyz/=a.w;
		fragPostionsV[i]=a.xyz;
		fragNormals[i]=unpackSnorm4x8(fragments[i].x).xyz;
		fragNormalsV[i]=mat3(uboCamera.V)*fragNormals[i];
		//fragDistanceAlongNormal[i]=uintBitsToFloat(fragments[i].z);
	}
	//=============
	if(fragCount==0)
	{
		outColor=vec4(0);
		return;
	}

	mat4 Vinv=inverse(uboCamera.V);

	vec3 N_V=fragNormals[fragCount-1];
	vec4 V_V=Pinv*vec4(p.xy,0,1);
	V_V.xyz/=V_V.w;
	V_V.xyz=-normalize(V_V.xyz);

	vec3 V=mat3(Vinv)*V_V.xyz;

	float cosTheta_i=min(dot(V_V.xyz,fragNormalsV[0]),1);
	vec3 T1_V=refract(-V_V.xyz,fragNormalsV[0],1./uPc.ior);
	vec3 T1=mat3(Vinv)*T1_V;	
	
	//vec3 E_t_in=textureLod(prefilteredEnvTex,fragNormals[fragCount-1],ROUGHNESS_1_LEVEL).rgb;
	vec3 E_t_in=textureLod(prefilteredEnvTex,T1,uPc.roughness*ROUGHNESS_1_LEVEL).rgb;
	vec3 F0=vec3(.04);

	//hemispherical directional reflectance
	vec3 col=E_t_in;

	float ior2=uPc.ior*uPc.ior;
	float ior4=ior2*ior2;

	for(int i=0;i+1<fragCount;i+=2)
	{
		if(bool(uPc.enableReflection))
		{
			vec3 tempR=reflect(-V.xyz,-fragNormals[i+1]);
			vec2 tempEnvBRDF=textureLod(envBrdfTex,vec2(abs(fragNormalsV[i+1].z),uPc.roughness),0).xy;
			col+=(tempEnvBRDF.x*F0+tempEnvBRDF.y)*textureLod(prefilteredEnvTex,tempR,uPc.roughness*ROUGHNESS_1_LEVEL).rgb;
		}
		vec3 fresnelFactor=vec3(1.);
			if(bool(uPc.fresnelEffect))
		fresnelFactor=(1-fresnelSchlick(abs(fragNormalsV[i].z),F0))*(1-fresnelSchlick(abs(fragNormalsV[i+1].z),F0));

		col*=exp(-uPc.extinctionFactor*uPc.extinction*(fragPostionsV[i].z-fragPostionsV[i+1].z))*fresnelFactor;
			
		if(bool(uPc.enableReflection))
		{
			vec3 tempR=reflect(-V.xyz,fragNormals[i]);
			vec2 tempEnvBRDF=textureLod(envBrdfTex,vec2(abs(fragNormalsV[i].z),uPc.roughness),0).xy;
			col+=(tempEnvBRDF.x*F0+tempEnvBRDF.y)*textureLod(prefilteredEnvTex,tempR,uPc.roughness*ROUGHNESS_1_LEVEL).rgb;

			//tempR=reflect(-V.xyz,-fragNormals[i+1]);
			//tempEnvBRDF=textureLod(envBrdfTex,vec2(abs(fragNormalsV[i+1].z),uPc.roughness),0).xy;
			//col+=(tempEnvBRDF.x*F0+tempEnvBRDF.y)*textureLod(prefilteredEnvTex,tempR,uPc.roughness*ROUGHNESS_1_LEVEL).rgb;
		}
	}
	outColor=vec4(col,1);
}