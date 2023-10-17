#version 460

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif

layout (location=0) in vec2 fs_texcoord;

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
}uboMaterial;

//only for obj 
layout(SET(2) binding=0) uniform sampler2D textures[8];

layout(SET(1) binding=0) buffer UBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
	int cascadeNum;
	float cascadeSplit[];
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

	vec4 vertices[4];
	mat4 PV[];
}uboLight;

void main()
{
	float alpha=1.;	
	if(uboMaterial.alpha_tex_index>=0)
		alpha=texture(textures[uboMaterial.alpha_tex_index],fs_texcoord).a;

	if(alpha<0.5)
		discard;
}