#version 460
#extension GL_EXT_scalar_block_layout :enable

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif
//layout(early_fragment_tests)in;

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
}uboMaterial;

//only for obj 
layout(SET(2) binding=0) uniform sampler2D textures[8];

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
//=============
mat3 surfaceTBN(vec3 n)
{
	vec3 UpVec= abs(n.z) < 0.999 ? vec3(0,0,1):vec3(1,0,0);
    //vec3 UpVec = abs(N.y) < 0.999 ? vec3(0,1,0):vec3(1,0,0);
    //vec3 UpVec=vec3(0,1,0);
    vec3 t=normalize(cross(UpVec,n));
    vec3 b = cross(n, t);
    return mat3(t, b, n);
}

void main()
{
	vec3 V= normalize(eyePos-fs_in.position);
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
	vec3 albedo=uboMaterial.diffuse;
	float metallic=uboMaterial.metallic;
	float roughness=uboMaterial.roughness;

	if(uboMaterial.bump_tex_index>=0)
	{
	 	N+=TBN*normalize(texture(textures[uboMaterial.bump_tex_index],fs_in.texcoord).xyz*2-1);
		N=normalize(N);
	}

	vec3 H=normalize(L+V);

	if(uboMaterial.diffuse_tex_index>=0)
		albedo=texture(textures[uboMaterial.diffuse_tex_index],fs_in.texcoord).rgb;
	if(uboMaterial.emissive_tex_index>=0)
		emissiveColor=texture(textures[uboMaterial.emissive_tex_index],fs_in.texcoord).rgb;
	if(uboMaterial.metallic_tex_index>=0)
		metallic=texture(textures[uboMaterial.metallic_tex_index],fs_in.texcoord).b;
	if(uboMaterial.roughness_tex_index>=0)
		roughness=texture(textures[uboMaterial.roughness_tex_index],fs_in.texcoord).g;
	
	float NdotL=clamp(dot(N,L),0,1.);
	float NdotH=clamp(dot(N,H),0,1);

	vec3 col=albedo*NdotL+ pow(NdotH,uboMaterial.shininess);
	outColor=vec4(uboLight.radiance*uboLight.color*col+albedo*0.2+emissiveColor,1);
}