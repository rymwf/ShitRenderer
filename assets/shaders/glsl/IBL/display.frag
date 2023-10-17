#version 460
#extension GL_EXT_scalar_block_layout :enable

#define PI 3.141592653

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
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

layout(SET(4) binding=2) uniform UBOother
{
	vec3 baseColor;
    float metallic2;
    float roughness2;
	int useManualBaseColor;
	int enableACESFilmic;
	int renderMode;
	int Roughness1Lod;
	int diffEnable;
	int specEnable;
	int multibounceEnable;
	int emissionEnable;
	float clearCoatThickness;
	float clearCoatRoughness;
};
layout(SET(4) binding=8) uniform samplerCube prefilteredEnvTex;
layout(SET(4) binding=9) uniform sampler2D brdfTex;

mat3 surfaceTBN(vec3 n)
{
	vec3 UpVec= abs(n.z) < 0.999 ? vec3(0,0,1):vec3(1,0,0);
    //vec3 UpVec = abs(N.y) < 0.999 ? vec3(0,1,0):vec3(1,0,0);
    //vec3 UpVec=vec3(0,1,0);
    vec3 t=normalize(cross(UpVec,n));
    vec3 b = cross(n, t);
    return mat3(t, b, n);
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
	vec3 V= normalize(uboCamera.eyePos-fs_position);
	vec3 R;
	vec3 L=vec3(1,0,0);
	vec3 N=fs_normal;
	mat3 TBN=surfaceTBN(N);
	vec3 albedo=uboMaterial.diffuse;
	vec3 emissiveColor=uboMaterial.emission;
	float ao=1.;
	float metallic=uboMaterial.metallic;
	float roughness=uboMaterial.roughness;

	if(uboMaterial.bump_tex_index>=0)
	{
	 	N+=TBN*normalize(texture(textures[uboMaterial.bump_tex_index],fs_texcoord).xyz*2-1);
		N=normalize(N);
	}

	R=reflect(-V,N);

	if(uboMaterial.diffuse_tex_index>=0)
	{
		albedo=texture(textures[uboMaterial.diffuse_tex_index],fs_texcoord).rgb;
	}
	
	if(uboMaterial.ambient_tex_index>=0)
		ao=texture(textures[uboMaterial.ambient_tex_index],fs_texcoord).r;
	if(uboMaterial.metallic_tex_index>=0)
		metallic=texture(textures[uboMaterial.metallic_tex_index],fs_texcoord).b;
	if(uboMaterial.roughness_tex_index>=0)
		roughness=texture(textures[uboMaterial.roughness_tex_index],fs_texcoord).g;

	if(bool(useManualBaseColor))
    {
		albedo=baseColor;
        metallic=metallic2;
        roughness=roughness2;
    }

	//brdf
	vec3 F0 = mix(vec3(0.04),albedo, metallic);

	//========================================
	if(uboMaterial.emissive_tex_index>=0)
		emissiveColor=texture(textures[uboMaterial.emissive_tex_index],fs_texcoord).rgb;

	vec3 E_spec=textureLod(prefilteredEnvTex,R,roughness*Roughness1Lod).rgb;
	vec3 E_diff=textureLod(prefilteredEnvTex,N,Roughness1Lod).rgb;

	float NdotV=max(dot(N,V),0);
	//hemispherical directional reflectance
	vec2 envBRDF=textureLod(brdfTex,vec2(NdotV,roughness),0).xy;
	vec3 RsF_V=envBRDF.x*F0+envBRDF.y;
	float RsF1_V=envBRDF.x+envBRDF.y;

	vec3 f_spec=F0*envBRDF.x+envBRDF.y;
	vec3 f_diff=albedo*(1-RsF_V)*(1-metallic);

	float RsF1_avg=textureLod(brdfTex,vec2(1,roughness),0).z;
	vec3 F_avg=(20.f*F0+1)/21.f;
	vec3 f_ms=F_avg*RsF1_avg*(1-RsF1_V)/(1-F_avg*(1-RsF1_avg));

	vec3 IBL_Lo=vec3(0);
	vec3 IBL_Lo_spec=E_spec*(f_spec+f_ms);
	vec3 IBL_Lo_diff=E_diff*f_diff;

	if(bool(diffEnable))
		IBL_Lo+=E_diff*f_diff;
	if(bool(specEnable))
		IBL_Lo+=E_spec*f_spec;
	if(bool(multibounceEnable))
		IBL_Lo+=E_spec*f_ms;
	if(bool(emissionEnable))
		IBL_Lo+=emissiveColor;

	//clear coat
	float Fc_V=0.04+0.96*pow(1-NdotV,5);
	float alpha_c=clearCoatRoughness*clearCoatRoughness;
	vec2 envBRDF_c=textureLod(brdfTex,vec2(NdotV,clearCoatRoughness),0).xy;
	float RsFc_V=envBRDF_c.x*0.04+envBRDF_c.y;
	float f_c_spec_ambient=RsFc_V;

	vec3 Lo_c=E_spec*f_c_spec_ambient;
	IBL_Lo*=1-clearCoatThickness*Fc_V;
	IBL_Lo+=clearCoatThickness*Lo_c;

	if(bool(enableACESFilmic))
		IBL_Lo=ACESFilm(IBL_Lo);
	outColor=vec4(IBL_Lo,1);
}