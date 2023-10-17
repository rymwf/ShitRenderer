#version 460
#extension GL_EXT_scalar_block_layout : enable

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
layout(location =3) in vec3 fs_eyePos;

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
};

//only for obj 
layout(SET(2) binding=0) uniform sampler2D textures[8];

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
	vec3 V=normalize(fs_eyePos-fs_position);
	//vec3 
	//
	vec3 L=vec3(1,0,0);
	if(uboLight.lightType==1)
	{
		L=mat3(uboLight.transformMatrix)*vec3(0,0,1);
	}

	vec3 N=fs_normal;
	mat3 TBN=surfaceTBN(N);

	vec3 emissiveColor=emission;
	vec3 albedo=diffuse;

	if(bump_tex_index>=0)
	{
	 	N+=TBN*normalize(texture(textures[bump_tex_index],fs_texcoord).xyz*2-1);
		N=normalize(N);
	}

	vec3 H=normalize(L+V);

	if(diffuse_tex_index>=0)
		albedo*=texture(textures[diffuse_tex_index],fs_texcoord).rgb;
	if(emissive_tex_index>=0)
		emissiveColor=texture(textures[emissive_tex_index],fs_texcoord).rgb;
	
	float NdotL=clamp(dot(N,L),0,1.);
	float NdotH=clamp(dot(N,H),0,1);

	vec3 col=albedo*NdotL+ pow(NdotH,shininess);
	outColor=vec4(uboLight.radiance*uboLight.color*col+emissiveColor,1);
}