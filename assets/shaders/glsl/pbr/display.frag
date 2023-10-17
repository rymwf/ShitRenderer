#version 460
#extension GL_EXT_scalar_block_layout :enable

#define PI 3.141592653

#ifdef VULKAN
#define SET(x)set=x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#extension GL_EXT_scalar_block_layout:enable
#define PUSH_CONSTANT std430,binding=2
#endif

layout(early_fragment_tests)in;

layout(location=0) out vec4 outColor;

layout(location =0) in vec3 fs_position;
layout(location =1) in vec3 fs_normal;
layout(location =2) in vec2 fs_texcoord;

layout(SET(2) binding=1) uniform Material 
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

	int shadingModel;
}uboMaterial;

//only for obj 
layout(SET(2) binding=0) uniform sampler2D textures[8];

layout(SET(1) binding=0) buffer UBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
}uboCamera;

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

layout(SET(4) binding=8) uniform sampler2D brdfTex;

layout(PUSH_CONSTANT) uniform uPushConstant
{
	int diffModel;
	int specModel;
	int multibounceModel;
	int enableAces;
	vec3 ambientRadiance;
	int ambientDiffModel;
	int ambientSpecModel;
	int ambientMultibounceModel;
}uPc;

//=============

vec3 ACESFilm(vec3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x*(a*x+b))/(x*(c*x+d)+e),0,1);
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
float NDF_BlinnPhong(float NdotH,float alphab)
{
	float shininess=2/(alphab*alphab+0.00001)-2;
	return step(0.,NdotH)*(shininess+2)*pow(NdotH,shininess)/(2*PI);	
}
float NDF_Beckmann(float NdotH,float alphab)
{
	float alphab2=alphab*alphab;
	float NdotH2=NdotH*NdotH;
	return step(0,NdotH)/(PI*alphab2*NdotH2*NdotH2+0.00001)*exp((NdotH2-1)/(alphab2*NdotH2+0.00001));
}
float a_shapeInvariant(float NdotS,float alpha)
{
	//S is V or L
	return NdotS/(alpha*sqrt(1-NdotS*NdotS)+0.0001);
}
float lambda_Beckmann(float a)
{
	float a2=a*a;
	return a>=1.6?0:(1-1.259*a+0.396*a2)/(3.535*a+2.181*a2);
}

float G1_Smith(float HdotS,float lambda)
{
	return step(0,HdotS)/(1+lambda);
}
float G2_Separable(float HdotL,float HdotV,float lambdaL,float lambdaV)
{
	return G1_Smith(HdotL,lambdaL)*G1_Smith(HdotV,lambdaV);
}
float G2_HeightCorrelated(float HdotL,float HdotV,float lambdaL,float lambdaV)
{
	return step(0,HdotL)*step(0,HdotV)/(1+lambdaL+lambdaV);
}
float G_Beckmann(float NdotL,float NdotV,float HdotL,float HdotV,float alpha)
{
	float lambdaL=lambda_Beckmann(a_shapeInvariant(NdotL,alpha));
	float lambdaV=lambda_Beckmann(a_shapeInvariant(NdotV,alpha));
	return G2_HeightCorrelated(HdotL,HdotV,lambdaL,lambdaV);
}
float G_GGX(float NdotL,float NdotV,float alpha)
{
	NdotL=abs(NdotL);
	NdotV=abs(NdotV);
	return 0.5*mix(2*NdotL*NdotV,NdotL+NdotV,alpha);///(4*NdotV*NdotL+0.00001);
}
//===========================================
vec3 specPhong(vec3 V,vec3 R,float shininess)
{
	return vec3(pow(clamp(dot(V,R),0,1),shininess));
}
float specBlinnPhong(float cosTheta,float shininess)
{
	return pow(cosTheta,shininess);
}
//======================
//scattering
vec3 diffLambertian(vec3 albedo)
{
	return albedo/PI;
}
vec3 diffLambertianPlus(vec3 F,vec3 albedo)
{
	return (1-F)*diffLambertian(albedo);
}
vec3 diffShirley(vec3 F0,vec3 albedo,float NdotL,float NdotV)
{
	return 21/(20*PI)*(1-F0)*albedo*(1-pow(1-NdotL,5))*(1-pow(1-NdotV,5));
}
//==========================================
//global variable
void main()
{
	vec3 V= normalize(uboCamera.eyePos-fs_position);
	vec3 R;
	vec3 L=vec3(1,0,0);
	vec3 N=fs_normal;
	vec3 H;
	mat3 TBN=surfaceTBN(N);
	float metallic=uboMaterial.metallic;
	float roughness=uboMaterial.roughness;
	vec3 emissiveColor=uboMaterial.emission;
	vec3 albedo=uboMaterial.diffuse;

	if(uboLight.lightType==1)
	{
		L=mat3(uboLight.transformMatrix)*vec3(0,0,1);
	}
	if(uboMaterial.bump_tex_index>=0)
	{
	 	N+=TBN*normalize(texture(textures[uboMaterial.bump_tex_index],fs_texcoord).xyz*2-1);
		N=normalize(N);
	}
	if(uboMaterial.diffuse_tex_index>=0)
		albedo*=texture(textures[uboMaterial.diffuse_tex_index],fs_texcoord).rgb;
	
	if(uboMaterial.metallic_tex_index>=0)
		metallic*=texture(textures[uboMaterial.metallic_tex_index],fs_texcoord).b;
	if(uboMaterial.roughness_tex_index>=0)
		roughness*=texture(textures[uboMaterial.roughness_tex_index],fs_texcoord).g;

	if(uboMaterial.emissive_tex_index>=0)
		emissiveColor=texture(textures[uboMaterial.emissive_tex_index],fs_texcoord).rgb;

	R=reflect(-V,N);
	H=normalize(L+V);

	float NdotH=dot(N,H);
	float HdotL=dot(H,L);
	float HdotV=max(dot(H,V),0);
	float NdotL=max(dot(N,L),0);
	float NdotV=dot(N,V);
	float LdotR=dot(L,R);

	//brdf
	vec3 F0 = mix(vec3(0.04),albedo, uboMaterial.metallic);
	vec3 F=fresnelSchlick(HdotL,F0);
	vec3 F_avg=(20*F0+1)/21.;
	vec3 F_V=fresnelSchlick(NdotV,F0);

	float alpha=roughness*roughness;

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

	//========================================

	vec3 f_diff=vec3(0);
	vec3 f_spec=vec3(0);
	vec3 f_ms=vec3(0.);
	//vec3 col_other=0;

	vec3 f_diff_ambient=vec3(0);
	vec3 f_spec_ambient=vec3(0);
	vec3 f_ms_ambient=vec3(0);

	switch(uPc.diffModel)
	{
		case 1:
		//lambertian
		f_diff=diffLambertian(albedo);
		break;
		case 2:
		f_diff=diffLambertianPlus(F,albedo);
		break;
		case 3:
		f_diff=diffShirley(F0,albedo,NdotL,NdotV);
		break;
		case 4:
		//AshikhminShirley
		f_diff=albedo*(1-RsF_L)*(1-RsF_V)/(PI*(1-RsF_avg)+0.1);
		break;
	}
	switch(uPc.specModel)
	{
		case 1:
		//blinn phong
		f_spec=vec3(pow(max(NdotH,0),uboMaterial.shininess));
		break;
		case 2:
		//phong
		f_spec=vec3(pow(max(LdotR,0),uboMaterial.shininess));
		break;
		case 3:
		//Beckmann
		f_spec=F*NDF_Beckmann(NdotH,alpha)*G_Beckmann(NdotL,NdotV,HdotL,HdotV,alpha);
		break;
		case 4:
		//blinnphong
		f_spec=F*NDF_BlinnPhong(max(NdotH,0),alpha)*G_Beckmann(NdotL,NdotV,HdotL,HdotV,alpha);
		break;
		case 5:
		//GGX
		f_spec=F*NDF_GGX(NdotH,alpha)*G_GGX(NdotL,NdotV,alpha);
		break;
	}
	switch(uPc.multibounceModel)
	{
		case 1:
		{
			float fms=(1-RsF1_L)*(1-RsF1_V)/(PI*(1-RsF1_avg)+0.1);
			vec3 k=F_avg*RsF1_avg/(1-F_avg*(1-RsF1_avg));
			f_ms=k*fms;
		}
		break;
	}

	switch(uPc.ambientDiffModel)
	{
		case 1:
		f_diff_ambient=albedo*(1-RsF_V);
		break;
	}
	switch(uPc.ambientSpecModel)
	{
		case 1:
		f_spec_ambient=RsF_V;
		break;
	}
	switch(uPc.ambientMultibounceModel)
	{
		case 1:
		{
			vec3 k=F_avg*RsF1_avg/(1-F_avg*(1-RsF1_avg));
			f_ms_ambient=k*(1-RsF1_V);
		}
		break;
	}
	//clear coat
	float Fc=0.04+0.96*pow(1-HdotL,5);
	float Fc_V=0.04+0.96*pow(1-NdotV,5);

	float alpha_c=uboMaterial.clearcoat_roughness*uboMaterial.clearcoat_roughness;
	float f_c_spec=Fc*NDF_GGX(NdotH,alpha_c)*G_GGX(NdotL,NdotV,alpha_c);
	vec2 envBRDF_c=textureLod(brdfTex,vec2(NdotV,uboMaterial.clearcoat_roughness),0).xy;
	float RsFc_V=envBRDF_c.x*0.04+envBRDF_c.y;
	float f_c_spec_ambient=RsFc_V;

	vec3 irradiance=uboLight.radiance*uboLight.color*max(dot(N,L),0)*PI;
	vec3 ambientIrradiance=uPc.ambientRadiance;//*PI;
	vec3 Lo=irradiance*(f_diff*(1-metallic)+f_spec+f_ms)+
	ambientIrradiance*(f_diff_ambient*(1-metallic)+f_spec_ambient+f_ms_ambient);

	vec3 Lo_c=irradiance*f_c_spec+ambientIrradiance*f_c_spec_ambient;

	Lo*=1-uboMaterial.clearcoat_thickness*Fc_V;
	Lo+=uboMaterial.clearcoat_thickness*Lo_c;

	//vec3 Lo=f_diff+f_spec +f_ms;
	//Lo/=2;
	if(bool(uPc.enableAces))
		Lo=ACESFilm(Lo);

	outColor=vec4(Lo,1);
}