#version 460

#ifdef VULKAN
#define SET(x) set=x,
#define SAMPLE_INPUT subpassInput 
#define SAMPLE_INPUT_MS subpassInputMS 
#define SAMPLE_FETCH(a,b,c) subpassLoad(a)
#define INPUT_ATTACHMENT_INDEX(x) input_attachment_index = x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#define SAMPLE_INPUT sampler2D 
#define SAMPLE_INPUT_MS sampler2DMS
#define SAMPLE_FETCH(a,b,c) texelFetch(a,b,c)
#define INPUT_ATTACHMENT_INDEX(x)
#define PUSH_CONSTANT binding=0
#endif

#define PI 3.141592653

layout (INPUT_ATTACHMENT_INDEX(0) binding = 1) uniform SAMPLE_INPUT inputPos;
layout (INPUT_ATTACHMENT_INDEX(1) binding = 2) uniform SAMPLE_INPUT inputAlbedo;
layout (INPUT_ATTACHMENT_INDEX(2) binding = 3) uniform SAMPLE_INPUT inputN;
layout (INPUT_ATTACHMENT_INDEX(3) binding = 4) uniform SAMPLE_INPUT inputMRA;
layout (INPUT_ATTACHMENT_INDEX(4) binding = 5) uniform SAMPLE_INPUT inputEmission;
layout (SET(3) binding = 6) uniform sampler2DArray shadowMap;

layout (PUSH_CONSTANT) uniform uPushConstant{
	int rendermode;
	int sampleCount;
};

layout (location = 0) out vec4 outColor;

layout(SET(1) binding=0) buffer UBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
	int cascadeNum;
	float cascadeSplit[];
}uboCamera;

layout(SET(2) binding=1) buffer UBOLight
{
	mat4 transformMatrix;
	vec3 color;
	int lightType; // 1 directional, 2 sphere, 3 spot, 4 tube, disable when <0
	vec3 radius;
	float rmax;
	float radiance;
	float cosThetaU;
	float cosThetaP;
	int vertexNum;
	vec3 vertices[4];
	mat4 PV[];
}uboLight;

//===============================
vec3 fresnelSchlick(float NdotL, vec3 F0)
{
    //F0 is the specular reflectance at normal incience
    //return F0 + (1.0 - F0) * pow(1.0 - clamp(NdotL,0,1), 5.0);
   return F0 + (1.0 - F0) * exp2((-5.55473 * NdotL- 6.98316) * NdotL);
}
float NDF_GGX(float NdotH, float alphag)
{
    float alphag2= alphag*alphag;
    float NdotH2 = NdotH*NdotH;
	float c=step(0.,NdotH);

    float nom   = alphag2;
    float denom = (NdotH2 * (alphag2- 1.0) + 1.0);

    //return alphag2;
    return max(alphag2,0.00001)*c/ (PI * denom * denom);
}
float G2_GGX(float NdotL,float NdotV,float alpha)
{
	//return G1_GGX(NdotL,alpha)*G1_GGX(NdotV,alpha);
	NdotL=abs(NdotL);
	NdotV=abs(NdotV);
	return 0.5*mix(2*NdotL*NdotV,NdotL+NdotV,alpha);//G2/(4*NdotV*NdotL)
}
vec3 GetBRDF_GGX(vec3 F0,vec3 N,vec3 L,vec3 V,float roughness)
{
	float alpha=roughness*roughness;
	vec3 H=normalize(L+V);
	float NdotH=dot(N,H);
	float HdotL=dot(H,L);
	float HdotV=dot(H,V);
	float NdotL=dot(N,L);
	float NdotV=dot(N,V);
	return fresnelSchlick(HdotL,F0)*G2_GGX(HdotL,HdotV,alpha)*NDF_GGX(NdotH,alpha);
	//return fresnelSchlick(NdotL,F0)*G2_GGX(NdotL,NdotV,alpha)*NDF_GGX(NdotH,alpha)/(4*NdotL*NdotV);
}

vec3 diffShirley(vec3 F0,vec3 albedo,vec3 N,vec3 L,vec3 V)
{
	float NdotL=max(dot(N,L),0);
	float NdotV=max(dot(N,V),0);
	return 21/(20*PI)*(1-F0)*albedo*(1-pow(1-NdotL,5))*(1-pow(1-NdotV,5));
}

float lightWin(float r,float rmax)
{
	float a=r/rmax;
	float b=max(1-pow(a,4),0);
	return b*b;
}
float lightFactorSpot(float r,float rmax,float cosTheta,float cosThetaU,float cosThetaP)
{
	float t=clamp((cosTheta-cosThetaU)/(cosThetaP-cosThetaU),0,1);
	return lightWin(r,rmax)*t*t;
}
float occDepthAvg(vec3 shadowCoord,vec2 ds,uint cascadeIndex)
{
	float dist=0.;
	int count=0;
	int range=1;
	for(int x=-range;x<=range;x++){
		for(int y=-range;y<=range;y++){
			float a=texture(shadowMap,vec3(shadowCoord.st+ds*vec2(x,y),cascadeIndex)).r;
			if(a<shadowCoord.z)
			{
				dist+=a;
				++count;
			}
		}
	}
	return count==0?0:dist/count;
}
float rand(vec2 co){
	return fract(sin(dot(co,vec2(12.9898,78.233)))*43758.5453);
}

mat2 rotate(float angle)
{
	float sinAngle=sin(angle);
	float cosAngle=cos(angle);
	return mat2(cosAngle,sinAngle,-sinAngle,cosAngle);
}
float filterHard(vec3 p,vec3 N,int cascadeIndex)
{
	float normalBiasFactor=0.01;
	vec3 shadowPos=p+N*normalBiasFactor;
	vec4 shadowCoord=uboLight.PV[cascadeIndex*2]*uboLight.PV[cascadeIndex*2+1]*vec4(shadowPos,1);
	shadowCoord.xyz/=shadowCoord.w;
	shadowCoord.xy=shadowCoord.xy*.5+.5;
	return float(texture(shadowMap,vec3(shadowCoord.xy,cascadeIndex)).r>shadowCoord.z);
}
float filterVSM(vec3 p,vec3 N,uint cascadeIndex)
{
	float normalBiasFactor=0.01;
	vec3 shadowPos=p+N*normalBiasFactor;
	vec4 shadowCoord=uboLight.PV[cascadeIndex*2]*uboLight.PV[cascadeIndex*2+1]*vec4(shadowPos,1);
	shadowCoord.xyz/=shadowCoord.w;
	shadowCoord.xy=shadowCoord.xy*.5+.5;
	
	ivec2 texDim=textureSize(shadowMap,0).xy;
	vec2 ds=1./vec2(texDim);

	//blocker search
	float scale=10.*shadowCoord.z;
	float d_occ=occDepthAvg(shadowCoord.xyz,ds*scale,cascadeIndex);
	if(d_occ==0)
		return 1.;

	float radius=(shadowCoord.z-d_occ)/shadowCoord.z*uboLight.rmax*10;
	//====================
	mat2 rotM=rotate(rand(shadowCoord.xy)*2*PI);
	float M1=0.,M2=0;
	
	int count=0;
	int range=2;
	for(int x=-range;x<=range;x++){
		for(int y=-range;y<=range;y++){
			float a=texture(shadowMap,vec3(shadowCoord.xy+rotM*radius*ds*vec2(x,y),cascadeIndex)).r;
			M1+=a;
			M2+=a*a;
			++count;
		}
	}
	M1/=count;
	M2/=count;
	
	if(M1>shadowCoord.z)
	return 1.;
	
	float var=abs(M2-M1*M1);
	float b=shadowCoord.z-M1;
	float pmax=var/(var+b*b);
	pmax=smoothstep(0.2,1,pmax);
	return pmax;
}
//=====================================
vec4 resolve(SAMPLE_INPUT inputAttachment)
{
	ivec2 uv=ivec2(gl_FragCoord.xy);
	return SAMPLE_FETCH(inputAttachment,uv,0);
}
vec3 render(int sampleIndex)
{
	ivec2 uv=ivec2(gl_FragCoord.xy);
	vec3 col=vec3(0);
	vec4 albedo=SAMPLE_FETCH(inputAlbedo,uv,sampleIndex);
	if(albedo.a==0)
		return col;

	vec3 pos=SAMPLE_FETCH(inputPos,uv,sampleIndex).rgb;
	vec3 N=SAMPLE_FETCH(inputN,uv,sampleIndex).rgb;
	vec3 MRA=SAMPLE_FETCH(inputMRA,uv,sampleIndex).rgb;
	vec3 emission=SAMPLE_FETCH(inputEmission,uv,sampleIndex).rgb;

	vec3 V=normalize(uboCamera.eyePos-pos);
	vec3 lightPos=uboLight.transformMatrix[3].xyz;
	vec3 L_dir=mat3(uboLight.transformMatrix)*vec3(0,0,-1);
	vec3 l=lightPos-pos;
	vec3 L=normalize(l);
	vec3 H=normalize(L+V);
	vec3 F0 = mix(vec3(0.04),albedo.rgb, MRA.x);

	vec3 Lo_spec=GetBRDF_GGX(F0,N,L,V,MRA.y);
	vec3 Lo_diff=diffShirley(F0,albedo.rgb,N,L,V);

	float lightFactor=lightFactorSpot(length(l),uboLight.rmax,dot(-L,L_dir),uboLight.cosThetaU,uboLight.cosThetaP);
	float shadowFactor=filterVSM(pos,N,0);
	//float shadowFactor=filterHard(pos,N,0);
	return (Lo_spec+Lo_diff*PI)*uboLight.radiance*uboLight.color*lightFactor*max(dot(N,L),0.)*MRA.z*shadowFactor;
}

void main() 
{
	vec4 col=vec4(0);
	// Apply brightness and contrast filer to color input
	switch(rendermode)
	{
		case 0:
		{
			col.rgb=render(0);
		}
		break;
		case 1:
			col = resolve(inputPos);
		break;
		case 2:
			col = resolve(inputAlbedo);
		break;
		case 3:
			col = resolve(inputN);
		break;
		case 4:
			col.rgb = vec3(resolve(inputMRA).r);
		break;
		case 5:
			col.rgb = vec3(resolve(inputMRA).g);
		break;
		case 6:
			col.rgb = vec3(resolve(inputMRA).b);
		break;
		case 7:
			col = resolve(inputEmission);
		break;
	}
	outColor=col;
}