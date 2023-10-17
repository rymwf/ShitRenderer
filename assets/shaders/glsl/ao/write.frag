#version 460
#extension GL_EXT_scalar_block_layout:enable

#ifdef VULKAN
#define SET(x)set=x,
#else
#define SET(x)
#endif
//layout(early_fragment_tests)in;

layout(location=0)out vec4 outPos;
layout(location=1)out vec3 outN;
layout(location=2)out vec4 outDiffuse;
layout(location=3)out vec4 outSpecularShininess;
layout(location=4)out vec4 outEmissionAO;

struct VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
};
layout(location=0)in VS_OUT fs_in;

layout(SET(2)binding=1)uniform Material
{
	vec3 ambient;
	float shininess;
	vec3 diffuse;
	float ior;// index of refraction
	vec3 specular;
	float dissolve;// 1 == opaque; 0 == fully transparent
	vec3 transmittance;
	float roughness;
	vec3 emission;
	float metallic;
	float sheen;// [0, 1] default 0
	float clearcoat_thickness;// [0, 1] default 0
	float clearcoat_roughness;// [0, 1] default 0
	float anisotropy;// aniso. [0, 1] default 0
	float anisotropy_rotation;// anisor. [0, 1] default 0
	
	int ambient_tex_index;// map_Ka
	int diffuse_tex_index;// map_Kd
	int specular_tex_index;// map_Ks
	int specular_highlight_tex_index;// map_Ns
	int bump_tex_index;// map_bump, map_Bump, bump
	int displacement_tex_index;// disp
	int alpha_tex_index;// map_d
	int reflection_tex_index;// refl
	
	int roughness_tex_index;//map_Pr
	int metallic_tex_index;//map_Pm
	int sheen_tex_index;//map_Ps
	int emissive_tex_index;//map_Ke
}uboMaterial;

//only for obj
layout(SET(2)binding=0)uniform sampler2D textures[8];

layout(SET(1)binding=0)buffer UBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
}uboCamera;

//=============
mat3 surfaceTBN(vec3 n)
{
	vec3 UpVec=abs(n.z)<.999?vec3(0,0,1):vec3(1,0,0);
	//vec3 UpVec = abs(N.y) < 0.999 ? vec3(0,1,0):vec3(1,0,0);
	//vec3 UpVec=vec3(0,1,0);
	vec3 t=normalize(cross(UpVec,n));
	vec3 b=cross(n,t);
	return mat3(t,b,n);
}

void main()
{
	vec3 N=fs_in.normal;
	mat3 TBN=surfaceTBN(N);
	
	vec4 emissionAO=vec4(uboMaterial.emission,1);
	vec4 albedo=vec4(uboMaterial.diffuse,uboMaterial.dissolve);
	//float metallic=uboMaterial.metallic;
	//float roughness=uboMaterial.roughness;
	vec4 specularShininess=vec4(uboMaterial.specular,uboMaterial.shininess);
	
	if(uboMaterial.bump_tex_index>=0)
	{
		N+=TBN*normalize(texture(textures[uboMaterial.bump_tex_index],fs_in.texcoord).xyz*2-1);
		N=normalize(N);
	}
	//if(uboMaterial.metallic_tex_index>=0)
	//	metallic=texture(textures[uboMaterial.metallic_tex_index],fs_in.texcoord).b;
	//if(uboMaterial.roughness_tex_index>=0)
	//	roughness=texture(textures[uboMaterial.roughness_tex_index],fs_in.texcoord).g;
	
	if(uboMaterial.diffuse_tex_index>=0)
	albedo=texture(textures[uboMaterial.diffuse_tex_index],fs_in.texcoord);
	if(uboMaterial.alpha_tex_index<0)
	albedo.a=uboMaterial.dissolve;
	if(uboMaterial.emissive_tex_index>=0)
	emissionAO.rgb=texture(textures[uboMaterial.emissive_tex_index],fs_in.texcoord).rgb;
	if(uboMaterial.specular_tex_index>=0)
	specularShininess.rgb=texture(textures[uboMaterial.specular_tex_index],fs_in.texcoord).rgb;
	if(uboMaterial.specular_highlight_tex_index>=0)
	specularShininess.a=texture(textures[uboMaterial.specular_highlight_tex_index],fs_in.texcoord).r;
	if(uboMaterial.ambient_tex_index>=0)
		emissionAO.a=texture(textures[uboMaterial.ambient_tex_index],fs_in.texcoord).r;
	
	outPos=vec4(fs_in.position,1);
	
	vec4 p=uboCamera.V*vec4(fs_in.position,1);
	//mat3 transM=inverse(mat3(1,0,0,0,1,0,-normalize(p)));

	outN=N;
	outDiffuse=albedo;
	outSpecularShininess=specularShininess;
	outEmissionAO=emissionAO;
}