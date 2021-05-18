#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif

layout(location = 0) out vec4 outColor;
layout(location = 0) in VS_OUT { 
	vec4 colorFactor; 
	vec2 texCoord;
	vec3 pos;
	vec3 normal;
}fs_in;

layout(binding=0 SET(2)) uniform sampler2D texAlbedo;
layout(binding=1 SET(2)) uniform sampler2D texNormal;
layout(binding=2 SET(2)) uniform sampler2D texMetallicRoughness;
layout(binding=3 SET(2)) uniform sampler2D texOcclusion;
layout(binding=4 SET(2)) uniform sampler2D texEmission;
layout(binding=5 SET(2)) uniform sampler2D texTransparency;

layout(binding=6 SET(4)) uniform samplerCube texReflection;
layout(binding=7 SET(4)) uniform samplerCube texIrradiance;
layout(binding=8 SET(4)) uniform sampler2D texEnvBRDF;

layout(binding=14 SET(2)) uniform UBOMaterial{
	vec3 emissiveFactor;
	float alphaCutoff;
	vec4 baseColorFactor;	
	float metallic;
	float roughness;
	float normalScale;
}uboMaterial;

struct Light{
	vec3 pos;
	vec4 color;
	vec4 intensity;
	vec2 lim_r; //the attenuation distance limit,r.x: min dist(sphere light radius), r.y: max dist
	vec3 direction;
	vec3 tube_p0;
	vec3 tube_p1;
};
layout(binding=12 SET(0)) uniform UBOFrame{
	mat4 PV;
	vec3 eyePosition;
	Light light;
	vec3 ambientColor;
};

layout(constant_id=3) const int maxRoughnessLevel=7;

mat3 surfaceTBN(vec3 N)
{
	vec3 up=abs(N.z)>0.999?vec3(1,0,0):vec3(0,0,1);
	vec3 T=cross(up,N);
	vec3 B=cross(N,T);
	return mat3(T,B,N);
}
//============================================================
//PBR
#define PI 3.141592653
#define K_ASHIKHMIN
//#define K_GINNEKEN

float k_Ashikhmin(float NdotS){float phi=1.-acos(NdotS);return 1.-exp(-7.3*phi*phi);}
float k_Ginneken(float NdotS){float phi=1.-acos(NdotS);return 4.41*phi/(4.41*phi+1.);}

#ifdef K_ASHIKHMIN
    #define K_G1(NdotS) k_Ashikhmin(NdotS) 
#elif defined K_GINNEKEN
    #define K_G1(NdotS) k_Ginneken(NdotS)
#endif

float BRDF_Alpha(float roughness){
    //roughness=max(0.001,roughness);
    return roughness*roughness;
}
//Disney modification for G
//can only be used for analytic shading, when used for IBL, it will be too dark  
float BRDF_Alpha_G(float roughness){
    return pow((roughness+1.)/2.,2.);
}
float BRDF_D(float alpha_g,float NdotH){
    float alpha_g2=alpha_g*alpha_g;
    float NdotH2=NdotH*NdotH;
    float deno=1.+NdotH2*(alpha_g2-1.);
    return step(0.,NdotH)*alpha_g2/(deno*deno*3.141592653);
}
vec3 BRDF_F0(vec3 albedo,float metallic){
    vec3 F0=vec3(0.04);    //debug
    return mix(vec3(F0), albedo, metallic);
}
vec3 BRDF_F(vec3 F0,float HdotL){
    //vec3 H=normalize(L+V);
    //vec3 HdotL=dot(H,L);
    // return R0+(1.-R0)*pow(1.-cosTheta,5.);   //Schlick Approximation
    return F0 + (1.0 - F0) * exp2((-5.55473 * HdotL - 6.98316) * HdotL);//Spherical Gaussian Approximation
}
float Lambda(float a,float NdotS){
    float a2=a*a;
    return (-1.+sqrt(1.+1./a2))/2.;
}
float Lambda_a(float alpha,float NdotS){
    return NdotS/(alpha*sqrt(1.-NdotS*NdotS));
}
float BRDF_G(float alpha,vec3 L,vec3 V,vec3 N){
    vec3 H=normalize(L+V);

    float NdotL=max(0.,dot(N,L));
    float NdotV=max(0.,dot(N,V));
    float HdotL=dot(H,L);
    float HdotV=dot(H,V);


    float lambda_a_L=Lambda_a(alpha,NdotL);
    float lambda_a_V=Lambda_a(alpha,NdotV);

    float lambda_L=Lambda(lambda_a_L,NdotL);
    float lambda_V=Lambda(lambda_a_V,NdotV);
    //Heitz direction and height correlated form
    float a=lambda_V<lambda_L?NdotV:NdotL;
    float G2=step(0.,HdotL)*step(0.,HdotV)/((1.+max(lambda_L,lambda_V)+K_G1(a)*min(lambda_L,lambda_V)));
    return G2;
}
//=====================================
//IBL
vec3 envBRDF(float NdotV,float roughness){return texture(texEnvBRDF,vec2(NdotV,roughness)).rgb;}
vec3 irrdianceEnv(vec3 N){return texture(texIrradiance,N).rgb;}
vec3 reflectionEnv(vec3 R,float roughness){float lod=roughness*maxRoughnessLevel;return textureLod(texReflection,R,lod).rgb;}

vec3 BRDF_F_avg(vec3 F0){return 20./21.*F0+1./21.;}

vec3 IBL_spec(vec3 F0,float NdotV,float roughness){
    vec3 env_brdf=envBRDF(NdotV,roughness);
    return F0*env_brdf.x+env_brdf.y;
}

vec3 IBL_diff(vec3 albedo,vec3 F0,float roughness,float NdotV,vec3 fs){
#if 1
    //
    vec3 RsF1_V=IBL_spec(vec3(1.),NdotV,roughness);
    vec3 F_avg=BRDF_F_avg(F0);
    float RsF1_avg=envBRDF(1,roughness).z;
    vec3 RsF_albedo_V=IBL_spec(albedo,NdotV,roughness);
    vec3 fms=albedo*(1.-RsF1_V);//*F1_avg*RsF1_avg/(1.-F1_avg*(1.-RsF1_avg));   //too dark when albedo is low
    vec3 fms2=(1.-RsF1_V)*F_avg*RsF1_avg/(1.-F_avg*(1.-RsF1_avg));
    vec3 fd=max(RsF_albedo_V-fs+fms-fms2,0.)+fms2;
#endif
#if 0
    vec3 RsF1_V=f_spec(vec3(1.),NdotV,roughness);
    vec3 F_albedo_avg=BRDF_F_avg(albedo);
    
    vec3 RsF_albedo_V=f_spec(albedo,NdotV,roughness);
    float RsF1_avg=getEnvBRDF(1,roughness).z;
    vec3 fms=(1.-RsF1_V)*F_albedo_avg*RsF1_avg/(1.-F_albedo_avg*(1.-RsF1_avg));     //color will be a little different when roughness is high 
    vec3 fd=max(RsF_albedo_V-fs+fms,0.);
#endif
#if 0
    vec3 RsF_albedo_V_r0=f_spec(albedo,NdotV,0.);
    vec3 fd=max(RsF_albedo_V_r0-fs,0.); //edge is too white
#endif
    return fd;
}

void main() 
{
	vec3 lightIntensity=light.intensity.rgb;
	vec4 texColor=texture(texAlbedo,fs_in.texCoord);

	vec3 albedo=pow(texColor.rgb,vec3(2.2))*uboMaterial.baseColorFactor.rgb*fs_in.colorFactor.rgb;
	vec3 emissiveColor=pow(texture(texEmission,fs_in.texCoord).rgb*uboMaterial.emissiveFactor,vec3(2.2));
	vec3 ao=texture(texOcclusion,fs_in.texCoord).rgb;

//	vec3 L=normalize(light.pos-fs_in.pos);
	vec3 L=normalize(-light.direction);
	vec3 N=normalize(surfaceTBN(fs_in.normal)*((texture(texNormal,fs_in.texCoord).rgb*2-1)*vec3(uboMaterial.normalScale,uboMaterial.normalScale,1.)));
	//vec3 N=fs_in.normal;
	vec3 V=normalize(eyePosition-fs_in.pos);
	vec3 R=reflect(-V,N);

	vec3 metallicRoughness=texture(texMetallicRoughness,fs_in.texCoord).rgb;
	float metallic=metallicRoughness.z*uboMaterial.metallic;
	float roughness=metallicRoughness.y*uboMaterial.roughness;

	//lambertian
	//outColor=vec4(albedo*(light.color.rgb*light.intensity.rgb*max(dot(L,N),0)+ambientColor)+emissiveColor,1);

	vec3 H=normalize(L+V);
    float NdotL=max(dot(N,L),0);
    float NdotV=max(dot(N,V),0);
    float HdotN=dot(H,N);
	float HdotL=dot(H,L);

	vec3 F0=BRDF_F0(albedo,metallic);
	vec3 F=BRDF_F(F0,HdotL);

    float alpha=BRDF_Alpha(roughness);
    float D=BRDF_D(alpha,HdotN);
    float alpha_G=BRDF_Alpha_G(roughness);
    float G=BRDF_G(alpha_G,L,V,N);

	vec3 f_spec=F*D*G/(4*NdotL*NdotV+0.001);
	vec3 f_diff=albedo/PI*(1.-F);	
	vec3 f_miss=vec3(0.);

	vec3 f_light=max(f_diff*(1-metallic)+f_spec+f_miss,0.);

	vec3 Lo_brdf=PI*lightIntensity*NdotL*f_light+emissiveColor+ambientColor*albedo;

	//==============================================================
	//IBL

    //prefiltered envmap
    vec3 prefilteredCol=reflectionEnv(R,roughness);    //col from hdr in linear space
    vec3 irradEnv=irrdianceEnv(N);
    vec3 fs=IBL_spec(F0,NdotV,roughness);
    vec3 fd=IBL_diff(albedo,F0,roughness,NdotV,fs);
	
    //return fs*IBL_switch.y+fd*IBL_switch.x;
    //return albedo;
    vec3 Lo_IBL= fs*prefilteredCol+fd*irradEnv*ao;
	outColor=vec4(Lo_brdf+Lo_IBL,1.);
	//outColor=vec4(Lo_brdf,texColor.a*uboMaterial.baseColorFactor.a);
	//outColor=vec4(Lo_brdf,texColor.a*uboMaterial.baseColorFactor.a);
	//outColor=vec4(albedo,1);
//outColor=fs_in.colorFactor;
}
