#version 460
#extension GL_EXT_scalar_block_layout :enable

#ifdef VULKAN
#define SET(x) set=x,
#define SUBPASS_INPUT subpassInput 
#define SUBPASS_INPUT_MS subpassInputMS 
#define SUBPASS_LOAD(tex,b,c) subpassLoad(tex)
#define INPUT_ATTACHMENT_INDEX(x) input_attachment_index = x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#define SUBPASS_INPUT sampler2D 
#define SUBPASS_INPUT_MS sampler2DMS
#define SUBPASS_LOAD(tex,xy,lod) texelFetch(tex,xy,lod)
#define INPUT_ATTACHMENT_INDEX(x)
#define PUSH_CONSTANT binding=0
#endif

layout(location=0) out vec4 outColor;

layout(SET(4)INPUT_ATTACHMENT_INDEX(0) binding=8) uniform SUBPASS_INPUT inputDepth;

struct VS_OUT
{
	vec3 position;
	vec3 normal;
	vec2 texcoord;
	float depth;
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

void main()
{
	ivec2 xy=ivec2(gl_FragCoord.xy);
	float d=SUBPASS_LOAD(inputDepth,xy,0).r;
	//peel near fragment
	if(fs_in.depth<=d)
		discard;
	gl_FragDepth=fs_in.depth;

	vec3 V= normalize(eyePos-fs_in.position);

	vec3 emissiveColor=uboMaterial.emission;
	vec4 albedo=vec4(uboMaterial.diffuse,uboMaterial.dissolve);

	if(uboMaterial.diffuse_tex_index>=0)
		albedo=texture(textures[uboMaterial.diffuse_tex_index],fs_in.texcoord);
	if(uboMaterial.alpha_tex_index>=0)
		albedo.a=texture(textures[uboMaterial.diffuse_tex_index],fs_in.texcoord).r;
	if(uboMaterial.emissive_tex_index>=0)
		emissiveColor=texture(textures[uboMaterial.emissive_tex_index],fs_in.texcoord).rgb;
	
	outColor.rgb=albedo.rgb+emissiveColor;
	outColor.a=0.6;
	outColor.rgb*=outColor.a;
}