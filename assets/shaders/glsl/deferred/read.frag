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

layout (INPUT_ATTACHMENT_INDEX(0) binding = 1) uniform SAMPLE_INPUT inputPos;
layout (INPUT_ATTACHMENT_INDEX(1) binding = 2) uniform SAMPLE_INPUT inputAlbedo;
layout (INPUT_ATTACHMENT_INDEX(2) binding = 3) uniform SAMPLE_INPUT inputN;
layout (INPUT_ATTACHMENT_INDEX(3) binding = 4) uniform SAMPLE_INPUT inputMRA;
layout (INPUT_ATTACHMENT_INDEX(4) binding = 5) uniform SAMPLE_INPUT inputEmission;
layout (SET(3) binding = 6) uniform sampler2DArrayShadow shadowTex;

#define PI 3.141592653

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
float calcShadow(vec3 p)
{
	float shadowFactor=1.;
	vec4 p_proj=uboLight.PV[0]*uboLight.PV[1]*vec4(p,1);
	p_proj/=p_proj.w;
	p_proj.xy=p_proj.xy*0.5+0.5;
	vec2 uv=p_proj.xy;
	shadowFactor= texture(shadowTex,vec4(uv,0,p_proj.z)).r;
	return shadowFactor;
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
	float shadowFactor=calcShadow(pos);
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