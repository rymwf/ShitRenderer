#version 460
#extension GL_EXT_scalar_block_layout:enable

#ifdef VULKAN
#define SET(x)set=x,
#else
#define SET(x)
#endif
//layout(early_fragment_tests)in;

#define PI 3.141592653

layout(location=0)out vec4 outColor;

layout(location=0)in vec3 fs_position;
layout(location=1)in vec3 fs_normal;
layout(location=2)in vec2 fs_texcoord;

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
	int cascadeNum;
	float cascadeSplit[];
}uboCamera;

layout(SET(3)binding=1)buffer UBOLight
{
	mat4 transformMatrix;
	vec3 color;
	int lightType;// 1 directional, 2 sphere, 3 spot, 4 tube, disable when <0
	vec3 radius;
	float rmax;
	
	float radiance;
	float cosThetaU;
	float cosThetaP;
	int vertexNum;
	
	vec4 vertices[4];
	mat4 PV[];
}uboLight;

layout(SET(4)binding=2)uniform UBOBuffer
{
	int renderMode;
	float normalBiasFactor;
	float shadowFilterRadius;
};
layout(SET(4)binding=8)uniform samplerCube shadowMap;

//=============
mat3 surfaceTBN(vec3 normal)
{
	vec3 n=normalize(normal);
	vec3 UpVec=abs(n.z)<.999?vec3(0,0,1):vec3(1,0,0);
	//vec3 UpVec = abs(N.y) < 0.999 ? vec3(0,1,0):vec3(1,0,0);
	//vec3 UpVec=vec3(0,1,0);
	vec3 t=normalize(cross(UpVec,n));
	vec3 b=cross(n,t);
	return mat3(t,b,n);
}
mat4 surfaceTBN(vec3 normal,vec3 o)
{
	vec3 n=normalize(normal);
	vec3 UpVec=abs(n.z)<.999?vec3(0,0,1):vec3(1,0,0);
	vec3 t=normalize(cross(UpVec,n));
	vec3 b=cross(n,t);
	return mat4(t,0,b,0,n,0,o,1);
}
float specBlinnPhong(float cosTheta,float shininess)
{
	return pow(cosTheta,shininess);
}
vec3 diffLambertian(vec3 albedo)
{
	return albedo/PI;
}

float rand(vec2 co){
	return fract(sin(dot(co,vec2(12.9898,78.233)))*43758.5453);
}

mat2 rotate(float angle)
{
	float sinAngle=sin(angle);
	float cosAngle=cos(angle);
	return mat2(cosAngle,sinAngle,-sinAngle,cosAngle);
}

float getShadow(vec3 shadowCoord,float dref)
{
	float dist=texture(shadowMap,shadowCoord).r;
	return float(dist>=dref);
}
float occDepthAvg(mat4 transM,float scale,float dref)
{
	float dist=0.;
	int count=0;
	int range=1;
	for(int x=-range;x<=range;x++){
		for(int y=-range;y<=range;y++){
			float a=texture(shadowMap,vec3(transM*vec4(scale*vec2(x,y)/range,0,1))).r;
			if(a<dref)
			{
				dist+=a;
				++count;
			}
		}
	}
	return count==0?0:dist/count;
}
float filterHard(vec3 p,vec3 N,vec3 lightPos)
{
	vec3 shadowPos=p+N*normalBiasFactor;
	vec3 shadowCoord=(shadowPos-lightPos);
	return getShadow(shadowCoord,length(shadowCoord)/uboLight.rmax);
}
float filterPCF(vec3 p,vec3 N,vec3 lightPos,float scale)
{
	vec3 shadowPos=p+N*normalBiasFactor;
	vec3 shadowCoord=(shadowPos-lightPos);
	float dref=length(shadowCoord)/uboLight.rmax;
	
	ivec2 texDim=textureSize(shadowMap,0);
	float dx=uboLight.rmax*PI/(2*texDim.x);
	mat2 rotM=rotate(rand(shadowCoord.xy)*2*PI);
	mat4 transM=surfaceTBN(shadowCoord,shadowCoord)*mat4(rotM);
	
	float shadowFactor=0;
	int count=0;
	int range=2;
	for(int x=-range;x<=range;x++){
		for(int y=-range;y<=range;y++){
			shadowFactor+=getShadow(vec3(transM*vec4(dx*scale*vec2(x,y)/range,0,1)),dref);
			count++;
		}
	}
	shadowFactor/=count;
	return shadowFactor;
}
float filterPCSS(vec3 p,vec3 N,vec3 lightPos)
{
	vec3 shadowPos=p+N*normalBiasFactor;
	vec3 shadowCoord=(shadowPos-lightPos);
	float dref=length(shadowCoord)/uboLight.rmax;
	
	ivec2 texDim=textureSize(shadowMap,0);
	float dx=uboLight.rmax*PI/(2*texDim.x);
	mat2 rotM=rotate(rand(shadowCoord.xy)*2*PI);
	mat4 transM=surfaceTBN(shadowCoord,shadowCoord)*mat4(rotM);
	
	//blocker search
	float d_occ=occDepthAvg(transM,dx*shadowFilterRadius,dref);
	if(d_occ==0)
	return 1.;
	float radius=(dref-d_occ)/dref*shadowFilterRadius;
	//=======================
	//do pcf
	float shadowFactor=0.;
	
	int count=0;
	int range=2;
	for(int x=-range;x<=range;x++){
		for(int y=-range;y<=range;y++){
			shadowFactor+=getShadow(vec3(transM*vec4(dx*radius*vec2(x,y)/range,0,1)),dref);
			count++;
		}
	}
	shadowFactor/=count;
	return shadowFactor;
}

float filterVSM(vec3 p,vec3 N,vec3 lightPos)
{
	vec3 shadowPos=p+N*normalBiasFactor;
	vec3 shadowCoord=shadowPos-lightPos;
	float dref=length(shadowCoord)/uboLight.rmax;
	
	ivec2 texDim=textureSize(shadowMap,0);
	float dx=uboLight.rmax*PI/(2*texDim.x);
	mat2 rotM=rotate(rand(shadowCoord.xy)*2*PI);
	mat4 transM=surfaceTBN(shadowCoord,shadowCoord)*mat4(rotM);
	
	//blocker search
	float d_occ=occDepthAvg(transM,dx*shadowFilterRadius,dref);
	if(d_occ==0)
	return 1.;
	float radius=(dref-d_occ)/dref*shadowFilterRadius;
	
	//====================
	float M1=0.,M2=0;
	int count=0;
	int range=2;
	for(int x=-range;x<=range;x++){
		for(int y=-range;y<=range;y++){
			float a=texture(shadowMap,vec3(transM*vec4(dx*radius*vec2(x,y)/range,0,1))).r;
			M1+=a;
			M2+=a*a;
			count++;
		}
	}
	M1/=count;
	M2/=count;
	
	if(M1>dref)
	return 1.;
	
	float var=M2-M1*M1;
	float b=dref-M1;
	float pmax=var/(var+b*b);
	pmax=smoothstep(.2,1,pmax);
	return pmax;
}

void main()
{
	vec3 V=normalize(uboCamera.eyePos-fs_position);
	
	vec3 N=fs_normal;
	mat4 TBN=surfaceTBN(N,fs_position);
	
	vec4 albedo=vec4(uboMaterial.diffuse,1);
	
	if(uboMaterial.bump_tex_index>=0)
	{
		N+=mat3(TBN)*normalize(texture(textures[uboMaterial.bump_tex_index],fs_texcoord).xyz*2-1);
		N=normalize(N);
	}
	
	if(uboMaterial.diffuse_tex_index>=0)
	albedo=texture(textures[uboMaterial.diffuse_tex_index],fs_texcoord);
	//if(uboMaterial.alpha_tex_index>=0)
	//	alpha=texture(textures[uboMaterial.alpha_tex_index],fs_texcoord).a;
	
	//=========================================
	vec3 fd=diffLambertian(albedo.rgb);
	vec3 fs=vec3(1.);
	
	vec3 Lo=vec3(0.);
	
	vec3 R=reflect(-V,N);
	vec3 lightPos=vec3(uboLight.transformMatrix[3]);
	vec3 l=lightPos-fs_position;
	vec3 L=normalize(l);
	vec3 H;
	float NdotL=0;
	
	float phongCostheta=1.;
	NdotL=max(dot(N,L),0);
	
	H=normalize(L+V);
	phongCostheta=clamp(dot(N,H),0,1);
	fs=uboMaterial.specular*specBlinnPhong(phongCostheta,uboMaterial.shininess);
	
	float shadowFactor=1.;
	switch(abs(renderMode))
	{
		case 1:
		shadowFactor=filterHard(fs_position,N,lightPos);
		break;
		case 2:
		shadowFactor=filterPCF(fs_position,N,lightPos,shadowFilterRadius);
		break;
		case 3:
		shadowFactor=filterPCSS(fs_position,N,lightPos);
		break;
		case 4:
		shadowFactor=filterVSM(fs_position,N,lightPos);
		break;
	}
	if(renderMode>0)
	Lo+=((fd*PI+fs)*uboLight.radiance*uboLight.color*NdotL)*shadowFactor;
	else
	Lo=vec3(shadowFactor);
	outColor=vec4(Lo,albedo.a);
}