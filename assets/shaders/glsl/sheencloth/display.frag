#version 460
#extension GL_EXT_scalar_block_layout :enable

layout(early_fragment_tests)in;

#ifdef VULKAN
#define SET(x)set=x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#extension GL_EXT_scalar_block_layout:enable
#define PUSH_CONSTANT std430,binding=2
#endif

#define PI 3.141592653

layout(location=0) out vec4 outColor;

struct VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
};
layout(location =0) in VS_OUT fs_in;

layout(SET(2) binding=1) uniform Mateiral 
{
	vec3 ambient;
	float shininess;
	vec3 diffuse;
	float ior;		// index of refraction
	vec3 specular;
	float dissolve; // 1 == opaque; 0 == fully transparent
	vec3 transmittance;
	float roughness;
	vec3 emission;
	float metallic;

	float sheen;			   // [0, 1] default 0
	float clearcoat_thickness; // [0, 1] default 0
	float clearcoat_roughness; // [0, 1] default 0
	float anisotropy;		   // aniso. [0, 1] default 0
	float anisotropy_rotation; // anisor. [0, 1] default 0

	int ambient_tex_index;			  // map_Ka
	int diffuse_tex_index;			  // map_Kd
	int specular_tex_index;			  // map_Ks
	int specular_highlight_tex_index; // map_Ns
	int bump_tex_index;				  // map_bump, map_Bump, bump
	int displacement_tex_index;		  // disp
	int alpha_tex_index;			  // map_d
	int reflection_tex_index;		  // refl

	int roughness_tex_index;		  //map_Pr
	int metallic_tex_index;			  //map_Pm
	int sheen_tex_index;			  //map_Ps
	int emissive_tex_index;			  //map_Ke

	vec3 offset;
	float rotateAngle;
	vec3 scale;
}uboMaterial;

//only for obj 
layout(SET(2) binding=0) uniform sampler2D textures[8];

layout(SET(4) binding=8) uniform sampler2D brdfTex;
layout(SET(4) binding=9) uniform sampler2D sheenBrdfTex;

layout(SET(1) binding=0) buffer UBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
};

layout(SET(3) binding=1) buffer UBOLight
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
}uboLight;

layout(PUSH_CONSTANT)uniform uPushConstant
{
	int renderMode;//bit 0: base layer ,1 sheen
	int enableAces;
	float sheenRoughnessFactor;
	vec3 ambientRadiance;
	vec4 sheenColor;
}uPc;


//=============
float max3(vec3 x)
{
	return max(max(x.x,x.y),x.z);
}

mat3 surfaceTBN(vec3 n)
{
	vec3 UpVec= abs(n.z) < 0.999 ? vec3(0,0,1):vec3(1,0,0);
    //vec3 UpVec = abs(N.y) < 0.999 ? vec3(0,1,0):vec3(1,0,0);
    //vec3 UpVec=vec3(0,1,0);
    vec3 t=normalize(cross(UpVec,n));
    vec3 b = cross(n, t);
    return mat3(t, b, n);
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

float sheenD(float NdotH,float alpha)
{
	float a=step(0,NdotH);
	float recpa=1./(alpha+0.00001);
	return a*(2+recpa)*pow(1-NdotH*NdotH,recpa*0.5)/(2.*PI);
}
float sheenL(float x,float alphaG)
{
	float oneMinusAlphaSq=(1.-alphaG)*(1.-alphaG);
	float a=mix(21.5473,25.3245,oneMinusAlphaSq);
	float b=mix(3.82987,3.32435,oneMinusAlphaSq);
	float c=mix(.19823,.16801,oneMinusAlphaSq);
	float d=mix(-1.97760,-1.27393,oneMinusAlphaSq);
	float e=mix(-4.32054,-4.85967,oneMinusAlphaSq);
	return a/(1.+b*pow(x,c))+d*x+e;
}
float sheenLambda(float cosTheta, float alphaG)
{
    return abs(cosTheta) < 0.5 ? exp(sheenL(cosTheta, alphaG)) : exp(2.0 * sheenL(0.5, alphaG) - sheenL(1.0 - cosTheta, alphaG));
}
float sheenLambda2(float cosTheta, float alphaG)
{
	return pow(sheenLambda(cosTheta,alphaG),1+2*pow(1-cosTheta,8));
}
float sheenG0(float NdotV,float NdotL)
{
	NdotV=max(NdotV,0);
	return 1/(4*(NdotL+NdotV-NdotL*NdotV));
}
float sheenG(float alphaG,float NdotV,float NdotL)
{
	NdotV=max(NdotV,0);
	//return  1.0 / ((1.0 + sheenLambda(NdotV, alphaG) + sheenLambda(NdotL, alphaG)) * (4.0 * NdotV * NdotL));
	return  1.0 / ((1.0 + sheenLambda2(NdotV, alphaG) + sheenLambda2(NdotL, alphaG)) * (4.0 * NdotV * NdotL+0.00001));
}
float sheenBRDFSimple(float alpha,float NdotH,float NdotV,float NdotL)
{
	return sheenD(NdotH,alpha)*sheenG0(NdotV,NdotL);
}

vec3 fresnelSchlick(float NdotL, vec3 F0)
{
	NdotL=clamp(NdotL,0,1);
    //F0 is the specular reflectance at normal incience
    //return F0 + (1.0 - F0) * pow(1.0 - clamp(NdotL,0,1), 5.0);
   return F0 + (1.0 - F0) * exp2((-5.55473 * NdotL- 6.98316) * NdotL);
}

vec3 ACESFilm(vec3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x*(a*x+b))/(x*(c*x+d)+e),0,1);
}

void main()
{
	vec3 V= normalize(eyePos-fs_in.position);

	vec2 uv=uboMaterial.scale.xy*fs_in.texcoord;

	//vec3 
	//
	vec3 L=vec3(1,0,0);
	if(uboLight.lightType==1)
	{
		L=mat3(uboLight.transformMatrix)*vec3(0,0,1);
	}

	vec3 N=fs_in.normal;
	mat3 TBN=surfaceTBN(N);

	vec3 emissiveColor=uboMaterial.emission;
	vec4 albedo=vec4(uboMaterial.diffuse,uboMaterial.dissolve);
	float metallic=uboMaterial.metallic;
	float roughness=uboMaterial.roughness;
	float ao=1.;
	vec4 sheenCol=uPc.sheenColor;

	if(uboMaterial.bump_tex_index>=0)
	{
	 	N+=TBN*normalize(texture(textures[uboMaterial.bump_tex_index],uv).xyz*2-1);
		N=normalize(N);
	}

	if(uboMaterial.diffuse_tex_index>=0)
		albedo.rgb=texture(textures[uboMaterial.diffuse_tex_index],uv).rgb;
	if(uboMaterial.emissive_tex_index>=0)
		emissiveColor=texture(textures[uboMaterial.emissive_tex_index],uv).rgb;
	if(uboMaterial.metallic_tex_index>=0)
		metallic=texture(textures[uboMaterial.metallic_tex_index],uv).b;
	if(uboMaterial.roughness_tex_index>=0)
		roughness=texture(textures[uboMaterial.roughness_tex_index],uv).g;
	if(uboMaterial.ambient_tex_index>=0)
		ao=texture(textures[uboMaterial.ambient_tex_index],uv).r;
	if(uboMaterial.sheen_tex_index>=0)
		sheenCol=texture(textures[uboMaterial.sheen_tex_index],uv);

	vec3 H=normalize(L+V);
	float NdotL=max(dot(N,L),0);
	float NdotH=dot(N,H);
	float NdotV=max(dot(N,V),0);
	float HdotL=dot(H,L);
	float HdotV=dot(H,V);

	vec3 F0 = mix(vec3(0.04),albedo.rgb, metallic);
	vec3 F=fresnelSchlick(HdotL,F0);
	vec3 F_avg=(20*F0+1)/21.;

	vec3 sheenF0 = mix(vec3(0.04),sheenCol.rgb, metallic);
	vec3 sheenF=fresnelSchlick(HdotL,sheenF0);
	vec3 sheenF_avg=(20*sheenF0+1)/21.;


	float alpha=roughness*roughness;
	float sheenRoughness=uPc.sheenRoughnessFactor*roughness;
	float sheenAlpha=sheenRoughness*sheenRoughness;

	//============================
	vec2 envBRDF_V=textureLod(brdfTex,vec2(NdotV,roughness),0).xy;
	vec2 envBRDF_L=textureLod(brdfTex,vec2(NdotL,roughness),0).xy;

	vec3 RsF_L=envBRDF_L.x*F0+envBRDF_L.y;
	vec3 RsF_V=envBRDF_V.x*F0+envBRDF_V.y;
	vec3 RsF_avg=vec3(
		textureLod(brdfTex,vec2(F.r,roughness),0).z,
		textureLod(brdfTex,vec2(F.g,roughness),0).z,
		textureLod(brdfTex,vec2(F.b,roughness),0).z);

	float RsF1_L=envBRDF_L.x+envBRDF_L.y;
	float RsF1_V=envBRDF_V.x+envBRDF_V.y;
	float RsF1_avg=textureLod(brdfTex,vec2(1,roughness),0).z;

	//==================
	vec2 sheenEnvBRDF_V=textureLod(sheenBrdfTex,vec2(NdotV,sheenRoughness),0).xy;
	vec2 sheenEnvBRDF_L=textureLod(sheenBrdfTex,vec2(NdotL,sheenRoughness),0).xy;

	float sheenRsF1_L=sheenEnvBRDF_L.x+sheenEnvBRDF_L.y;
	float sheenRsF1_V=sheenEnvBRDF_V.x+sheenEnvBRDF_V.y;
	float sheenRsF1_avg=textureLod(sheenBrdfTex,vec2(1,sheenRoughness),0).z;

	//===============

	//base layer
	vec3 irradiance=uboLight.color*uboLight.radiance*NdotL*PI;
	vec3 ambientIrradiance=uPc.ambientRadiance;

	vec3 f_diff=albedo.rgb*(1-RsF_L)*(1-RsF_V)/(PI*(1-RsF_avg)+0.1);
	vec3 f_spec=F*G2_GGX(HdotL,HdotV,alpha)*NDF_GGX(NdotH,alpha);
	vec3 k=F_avg*RsF1_avg/(1-F_avg*(1-RsF1_avg));
	vec3 f_ms=k*(1-RsF1_L)*(1-RsF1_V)/(PI*(1-RsF1_avg)+0.1);

	vec3 f_diff_ambient=albedo.rgb*(1-RsF_V);
	vec3 f_spec_ambient=RsF_V;
	vec3 f_ms_ambient=k*(1-RsF1_V);

	vec3 f_baseLayer=f_diff*(1-metallic)+f_spec+f_ms;
	vec3 f_baseLayer_ambient=f_diff_ambient*(1-metallic)+f_spec_ambient+f_ms_ambient;

	//sheen layer
	float f_sheen=sheenD(NdotH,sheenAlpha)*sheenG(sheenAlpha,NdotV,NdotL);
	float f_sheen_ambient=sheenRsF1_V;

	//sheen col
	float rho=max(max(sheenCol.r,sheenCol.g),sheenCol.b);
	float sheenAlbedoScaling=min(1.-rho*sheenRsF1_V,1-rho*sheenRsF1_L);
	float sheenAlbedoScalingAmbient=1.-rho*sheenRsF1_V;

	vec3 Lo=vec3(0);
	switch(uPc.renderMode)
	{
		case 1:
		Lo=f_baseLayer*irradiance+f_baseLayer_ambient*ambientIrradiance;
		break;
		case 2:
		Lo=sheenCol.rgb*(f_sheen*irradiance+f_sheen_ambient*ambientIrradiance);
		break;
		case 3:
		Lo=irradiance*mix(f_baseLayer*sheenAlbedoScaling,f_sheen*sheenCol.rgb,sheenCol.a)+
		ambientIrradiance*mix(f_baseLayer_ambient*sheenAlbedoScalingAmbient,f_sheen_ambient*sheenCol.rgb,sheenCol.a);
		break;
	}
	if(bool(uPc.enableAces))
		Lo=ACESFilm(Lo);
	outColor=vec4(Lo,1.);
}