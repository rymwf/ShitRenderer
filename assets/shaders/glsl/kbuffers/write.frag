#version 460
#extension GL_EXT_scalar_block_layout :enable

//layout(early_fragment_tests) in;

#ifdef VULKAN
#define SET(x) set=x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#define PUSH_CONSTANT binding=0
#endif

struct VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
};
layout(location =0) in VS_OUT fs_in;

layout(SET(4)binding=0,r32ui)uniform uimage2D counterImage;
layout(SET(4)binding=1,rg32ui)uniform writeonly uimage2DArray outImages;

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

void main()
{
	vec3 emissiveColor=uboMaterial.emission;
	vec4 albedo=vec4(uboMaterial.diffuse,uboMaterial.dissolve);

	if(uboMaterial.diffuse_tex_index>=0)
		albedo=texture(textures[uboMaterial.diffuse_tex_index],fs_in.texcoord);
	if(uboMaterial.alpha_tex_index>=0)
		albedo.a=texture(textures[uboMaterial.diffuse_tex_index],fs_in.texcoord).r;
	if(uboMaterial.emissive_tex_index>=0)
		emissiveColor=texture(textures[uboMaterial.emissive_tex_index],fs_in.texcoord).rgb;
	
	vec3 col=albedo.rgb+emissiveColor;

	uint index=imageAtomicAdd(counterImage,ivec2(gl_FragCoord.xy),1);
	uvec4 item=uvec4(0);
	item.x=packUnorm4x8(vec4(col,.7));
	item.y=floatBitsToUint(gl_FragCoord.z);

	imageStore(outImages,ivec3(gl_FragCoord.xy,index),item);
}