#version 460
#extension GL_EXT_scalar_block_layout :enable

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif
//layout(early_fragment_tests)in;

#define PI 3.141592653

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
mat4 surfaceTBN(vec3 n,vec3 o)
{
	vec3 UpVec= abs(n.z) < 0.999 ? vec3(0,0,1):vec3(1,0,0);
    //vec3 UpVec = abs(N.y) < 0.999 ? vec3(0,1,0):vec3(1,0,0);
    //vec3 UpVec=vec3(0,1,0);
    vec3 t=normalize(cross(UpVec,n));
    vec3 b = cross(n, t);
    return mat4(t,0, b,0, n,0,o,1);
}

mat3 scale(vec3 s)
{
	mat3 ret=mat3(1);
	ret[0][0]=s.x;
	ret[1][1]=s.y;
	ret[2][2]=s.z;
	return ret;
}
//wxyz
mat3 quat2mat3(vec4 q)
{
	mat3 Result=mat3(1);
	float qxx=q.x * q.x;
	float qyy=q.y * q.y;
	float qzz=q.z * q.z;
	float qxz=q.x * q.z;
	float qxy=q.x * q.y;
	float qyz=q.y * q.z;
	float qwx=q.w * q.x;
	float qwy=q.w * q.y;
	float qwz=q.w * q.z;

	Result[0][0] = float(1) - float(2) * (qyy +  qzz);
	Result[0][1] = float(2) * (qxy + qwz);
	Result[0][2] = float(2) * (qxz - qwy);

	Result[1][0] = float(2) * (qxy - qwz);
	Result[1][1] = float(1) - float(2) * (qxx +  qzz);
	Result[1][2] = float(2) * (qyz + qwx);

	Result[2][0] = float(2) * (qxz + qwy);
	Result[2][1] = float(2) * (qyz - qwx);
	Result[2][2] = float(1) - float(2) * (qxx +  qyy);
	return Result;
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
float NDF_BlinnPhong(float NdotH,float alphap)
{
	return step(0,NdotH)*(alphap+2)*pow(NdotH,alphap)/(2*PI);
}
float G2_GGX(float NdotL,float NdotV,float alpha)
{
	//return G1_GGX(NdotL,alpha)*G1_GGX(NdotV,alpha);
	NdotL=abs(NdotL);
	NdotV=abs(NdotV);
	return 0.5*mix(2*NdotL*NdotV,NdotL+NdotV,alpha);//G2/(4*NdotV*NdotL)
}
vec3 GetBRDF(vec3 F0,vec3 N,vec3 L,vec3 V,float roughness)
{
	float alpha=roughness*roughness;
	vec3 H=normalize(L+V);
	float NdotH=dot(N,H);
	float HdotL=dot(H,L);
	float HdotV=dot(H,V);
	float NdotL=dot(N,L);
	float NdotV=dot(N,V);
	return fresnelSchlick(HdotL,F0)*G2_GGX(HdotL,HdotV,alpha)*NDF_GGX(NdotH,alpha);
	//return fresnelSchlick(NdotL,F0)*G2_GGX(NdotL,NdotV,alpha)*NDF_GGX(NdotH,alpha)/(4*NdotL*NdotV);
}
float specBlinnPhong(float cosTheta,float shininess)
{
	return pow(cosTheta,shininess);
}
vec3 diffShirley(vec3 F0,vec3 albedo,vec3 N,vec3 L,vec3 V)
{
	float NdotL=max(dot(N,L),0);
	float NdotV=max(dot(N,V),0);
	return 21/(20*PI)*(1-F0)*albedo*(1-pow(1-NdotL,5))*(1-pow(1-NdotV,5));
}
vec3 diffLambertian(vec3 albedo)
{
	return albedo/PI;
}
//float lineLightIntensity(float t,float A,float B,float C,float D,float E)
//{
//	return 2*((2*A*E-B*D)*t+A*D-2*B*C)/((4*C*E-D*D)*sqrt(E*t*t+D*t+C));
//}
float lineLightIntegral(vec3 l0,vec3 ld)
{
	vec3 N=vec3(0,0,1);
	float len=length(ld);
	float A=dot(l0,N)*len;
	float B=dot(ld,N)*len;
	if(A<=0&&dot(l0+ld,N)<=0)
		return 0;
	
	float t=-A/B;
	if(t>0&&t<1)
	{
		if(A<=0)
		{
			l0+=t*ld;
			ld*=(1-t);
		}
		else
			ld*=t;
		len=length(ld);
		A=dot(l0,N)*len;
		B=dot(ld,N)*len;
	}
	float C=dot(l0,l0);
	float D=2*dot(ld,l0);
	float E=dot(ld,ld);

	float p=sqrt(C);
	float q=sqrt(C+D+E);

	return (p*(2*D*(A-B)+4*(A*E-B*C))+q*(4*B*C-2*A*D))/(q*p*(4*C*E-D*D));
}
vec2 lineLightIntegral(vec3 l0,vec3 ld,vec3 N,vec3 R)
{
	float len=length(ld);
	float A=dot(l0,N)*len;
	float B=dot(ld,N)*len;
	if(A<=0&&dot(l0+ld,N)<=0)
		return vec2(0);
	
	float t=-A/B;
	if(t>0&&t<1)
	{
		if(A<=0)
		{
			l0+=t*ld;
			ld*=(1-t);
		}
		else
			ld*=t;
		len=length(ld);
		A=dot(l0,N)*len;
		B=dot(ld,N)*len;
	}
	float C=dot(l0,l0);
	float D=2*dot(ld,l0);
	float E=dot(ld,ld);

	//spec, most representive point
	float P=dot(R,l0);
	float Q=dot(R,ld);

	t=clamp((D*P-2*C*Q)/(D*Q-2*E*P),0,1);

	float p=sqrt(C);
	float q=sqrt(C+D+E);
	return vec2((p*(2*D*(A-B)+4*(A*E-B*C))+q*(4*B*C-2*A*D))/(q*p*(4*C*E-D*D)),t);
}
float areaLightLineIntegral(vec3 N,vec3 l0,vec3 l1)
{
	vec3 nl=cross(l1,l0);
	if(length(nl)<0.001)
		return 0;
	return dot(normalize(nl),N)*acos(dot(l1,l0));
}
float areaLightIntegral(int vertexNum,vec3 vertices[4])
{
	bool belowHorizon[4];
	vec3 l[4];
	int count=0;
	vec3 N=vec3(0,0,1);

	l[0]=normalize(vertices[0]);
	belowHorizon[0]=(dot(l[0],N)<0);
	if(!belowHorizon[0])
		++count;
	
	int i=1;
	vec3 templ;
	for(i=1;i<vertexNum;++i)
	{
		templ=normalize(vertices[i]);
		belowHorizon[i]=(dot(templ,N)<0);
		if(belowHorizon[i]^^belowHorizon[i-1])
		{
			vec3 v=vertices[i]-vertices[i-1];
			l[count++]=normalize(vertices[i-1]+dot(-vertices[i-1],N)/dot(v,N)*v);
		}
		if(!belowHorizon[i])
			l[count++]=templ;
	}
	if(belowHorizon[0]^^belowHorizon[i-1])
	{
		vec3 v=vertices[0]-vertices[i-1];
		l[count++]=normalize(vertices[i-1]+dot(-vertices[i-1],N)/dot(v,N)*v);
	}
	float res=0;
	for(int i=0;i<(count-1);++i)
		res+=areaLightLineIntegral(N,l[i],l[i+1]);
	if(count>2)
		res+=areaLightLineIntegral(N,l[count-1],l[0]);
	return res;
}

void main()
{
	vec3 V= normalize(eyePos-fs_position);

	vec3 N=fs_normal;
	mat4 TBN=surfaceTBN(N,fs_position);

	vec3 emissionColor=uboMaterial.emission;
	vec3 albedo=uboMaterial.diffuse;

	vec3 F0;
	float metallic=uboMaterial.metallic;
	float roughness=uboMaterial.roughness;

	if(uboMaterial.bump_tex_index>=0)
	{
	 	N+=mat3(TBN)*normalize(texture(textures[uboMaterial.bump_tex_index],fs_texcoord).xyz*2-1);
		N=normalize(N);
	}

	if(uboMaterial.diffuse_tex_index>=0)
		albedo=texture(textures[uboMaterial.diffuse_tex_index],fs_texcoord).rgb;
	if(uboMaterial.emissive_tex_index>=0)
		emissionColor=texture(textures[uboMaterial.emissive_tex_index],fs_texcoord).rgb;
	if(uboMaterial.metallic_tex_index>=0)
		metallic*=texture(textures[uboMaterial.metallic_tex_index],fs_texcoord).b;
	if(uboMaterial.roughness_tex_index>=0)
		roughness*=texture(textures[uboMaterial.roughness_tex_index],fs_texcoord).g;

	//=========================================
	F0 = mix(vec3(0.04),albedo, uboMaterial.metallic);
	vec3 fd=(1-metallic)*diffLambertian(albedo);
	vec3 fs=vec3(1.);

	vec3 Lo=emissionColor;

	vec3 R=reflect(-V,N);
	vec3 L=vec3(1,0,0);
	vec3 H=vec3(1,0,0);
	float NdotL=0;

	vec3 pl=vec3(uboLight.transformMatrix[3]);

	float phongCostheta=1.;
	switch(uboLight.lightType)
	{
		case 1:
		{
			//directional
			L=-pl;
			NdotL=max(dot(N,L),0);

			H=normalize(L+V);
			phongCostheta=clamp(dot(N,H),0,1);
			fs=uboMaterial.specular*specBlinnPhong(phongCostheta,uboMaterial.shininess);

			Lo+=(fd*PI+fs)*uboLight.radiance*uboLight.color*NdotL;
			break;
		}
		case 2:
		{
			//sphere
			vec3 l=pl-fs_position;
			float lightIntensity=uboLight.radiance*PI*uboLight.radius.x*uboLight.radius.x/(dot(l,l)+0.01);
			float Rdotl=dot(R,l);
			vec3 pcr=Rdotl*R-l;
			vec3 pcs=pl+pcr*min(1,uboLight.radius.x/length(pcr));
			L=normalize(pcs-fs_position);
			NdotL=dot(N,L);

			H=normalize(L+V);
			phongCostheta=clamp(dot(N,H),0,1);
			fs=uboMaterial.specular*specBlinnPhong(phongCostheta,uboMaterial.shininess);
			Lo+=(fd+fs)*uboLight.color*lightIntensity*max(NdotL,0);
			break;
		}
		case 3:
		{
			break;
		}
		case 4:
		{
			//tube
			vec3 pl1=mat4x3(uboLight.transformMatrix)*vec4(1,0,0,1);
#if 1
			mat4x3 TBN_inv=mat4x3(inverse(TBN));
			vec3 l0=TBN_inv*vec4(pl,1);
			vec3 ld=TBN_inv*vec4(pl1,1)-l0;
			vec3 Lo_diff=fd*uboLight.color*uboLight.radiance*lineLightIntegral(l0,ld);

			//TODO: fitting
			float s=1/sqrt(2/(uboMaterial.shininess+1));
			mat3 M_inv= scale(vec3(s,s,1));
			mat4x3 TBR_inv=mat4x3(inverse(surfaceTBN(R,fs_position)));
			TBR_inv=M_inv*TBR_inv;
			vec3 l0r=TBR_inv*vec4(pl,1);	
			vec3 ldr=TBR_inv*vec4(pl1,1)-l0r;	
			vec3 Lo_spec=uboLight.color*uboLight.radiance*lineLightIntegral(l0r,ldr);
#else
			vec3 l0=pl-fs_position;
			vec3 ld=pl1-pl;

			vec2 res=lineLightIntegral(l0,ld,N,R);

			vec3 l=l0+res.y*ld;
			L=normalize(l);
			H=normalize(L+V);
			fs=uboMaterial.specular*specBlinnPhong(max(dot(N,H),0),uboMaterial.shininess);
			vec3 Lo_diff=fd*uboLight.color*uboLight.radiance*res.x*50;
			vec3 Lo_spec=fs*uboLight.color*uboLight.radiance;
#endif
			Lo+=Lo_diff+Lo_spec;
			break;
		}
		case 5:
		{
			//disk, most representive point
			vec3 nl=normalize(mat3x3(uboLight.transformMatrix)*vec3(0,1,0));
			vec3 ppl=pl-fs_position;
			float a=dot(ppl,nl);
			if(a<0)
			{
				vec3 h=normalize(N-nl);
				vec3 ph=fs_position+dot(ppl,nl)/dot(h,nl)*h;
				vec3 pd=pl+normalize(ph-pl)*min(uboLight.radius.x,length(ph-pl));
				vec3 l=pd-fs_position;
				L=normalize(l);
				NdotL=max(dot(N,L),0);

				//vec3 h2=normalize(R-nl);
				vec3 pr=fs_position+dot(ppl,nl)/dot(R,nl)*R;
				vec3 pd2=pl+normalize(pr-pl)*min(uboLight.radius.x,length(pr-pl));
				vec3 lr=pd2-fs_position;
				vec3 Lr=normalize(lr);
				H=normalize(Lr+V);
				fs=uboMaterial.specular*specBlinnPhong(max(dot(N,H),0),uboMaterial.shininess);

				float A=uboLight.radius.x*uboLight.radius.z;
				vec3 Lo_diff=fd*uboLight.color*uboLight.radiance*max(NdotL,0)*max(dot(-nl,L),0)*A/dot(l,l);
				vec3 Lo_spec=PI*fs*uboLight.color*uboLight.radiance*max(dot(N,Lr),0)*max(dot(-nl,L),0);//*A/dot(lr,l);
				Lo+=Lo_diff+Lo_spec;
			}
			break;
		}
		case 6:
		{
			//quad
			vec3 nl=normalize(mat3x3(uboLight.transformMatrix)*vec3(0,1,0));
			vec3 ppl=pl-fs_position;
			float a=dot(ppl,nl);
			if(a>=0)
				break;

			mat4x3 TBN_inv=mat4x3(inverse(TBN));
			vec3 vertices0[4];
			for(int i=0;i<uboLight.vertexNum;++i)
				vertices0[i]=TBN_inv*vec4(uboLight.vertices[i],1);

			//TODO: fitting
			float s=1/sqrt(2/(uboMaterial.shininess+1));
			mat3 M_inv= scale(vec3(s,s,1));
			mat4x3 TBR_inv=mat4x3(inverse(surfaceTBN(R,fs_position)));
			vec3 vertices1[4];
			for(int i=0;i<uboLight.vertexNum;++i)
				vertices1[i]=M_inv*TBR_inv*vec4(uboLight.vertices[i],1);

			float res_d=areaLightIntegral(uboLight.vertexNum,vertices0);
			float res_s=areaLightIntegral(uboLight.vertexNum,vertices1);
			Lo+=uboLight.color*uboLight.radiance/2*(fd*res_d+res_s/PI);
			break;
		}
	}
	//phongCostheta=max(dot(R,L),0);

	//vec3 f=GetBRDF(F0,N,L,V,roughness)+(1-metallic)*diffShirley(F0,albedo,N,L,V);
	//vec3 f=(1-metallic)*diffLambertian(albedo)+specPhong(phongCostheta,500);//lambertian+blinnphong
	//vec3 f=specPhong(phongCostheta,uboMaterial.shininess);
	outColor=vec4(Lo,1);
	//outColor=vec4(irradiance,1);
	//outColor=vec4(vec3(NdotL),1);
	//outColor=vec4(0,0,0,1);
}