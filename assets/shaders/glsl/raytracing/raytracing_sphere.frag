#version 460
layout(location=0) out vec4 fragColor;
//layout(location=0) out float fragColor;

layout(binding=0) uniform UBO
{
	vec3 iResolution;
	float iTime;
	vec4 iChannelTime;
	vec4 iMouse;
	vec4 iDate;
	vec3 iChannelResolution[4];
	float iTimeDelta;
	float iFrame;
	float iSampleRate;
};

layout(binding=1) uniform sampler2D textures[4];

//====================================
#define PI 3.14159265358979323846
#define RAY_MAXDEPTH 100.
#define DEFAULT_OBJ Obj(RAY_MAXDEPTH,-1)
#define ZERO (min(iFrame,0))
#define CAMERA_DIST 2.3

const vec3 sc0=vec3(0.,0.,0.);  //sphere center
const vec3 groundOffset=vec3(0.,-1,0.);   //

struct Light{
	vec3 direction;
	vec3 intensity;
};

Light directionalLight=Light(vec3(-1,-1,0),vec3(1));
//Light directionalLight=Light(vec3(0,-0.3,-1),vec3(0,-1,0));

struct Obj{
	float depth;
	int id;
};
struct Material{
	vec3 kambiendt;	//ambient
	vec3 kd;	//diffuse
	vec3 ks;	//specular
	vec3 km;
	float shinness;	//shinees
};
struct MaterialPBR{
	float metallic;
	float roughness;
	float ambientOcclusion;
};
struct ObjAttribute
{
	vec3 N;
	vec2 texCoord;
	vec3 albedo;
	Material material;
	MaterialPBR materialPbr;
};

vec2 getUV()
{
	vec2 uv=(gl_FragCoord.xy+vec2(0.5))/iResolution.xy;
	uv-=0.5;
#ifdef VULKAN
	uv.y*=-1;
#endif
	if(iResolution.x>iResolution.y)
		uv.x*=iResolution.x/iResolution.y;
	else
		uv.y*=iResolution.y/iResolution.x;
	return uv;
}

mat3 surfaceTBN(vec3 N){
	vec3 UpVec= abs(N.z) < 0.999 ? vec3(0,0,1):vec3(1,0,0);
	//vec3 UpVec = abs(N.y) < 0.999 ? vec3(0,1,0):vec3(1,0,0);
	//vec3 UpVec=vec3(0,1,0);
	vec3 T=normalize(cross(UpVec,N));
	vec3 B = cross(N, T);
	return mat3(T, B, N);
}

//eye matrix
mat3 lookAt(vec3 pos,vec3 center,vec3 up){
	mat3 m;
	//eye matrix
	vec3 ww=-normalize(center-pos);
	vec3 uu=normalize(cross(up,ww));
	vec3 vv=cross(ww,uu);
	m[0]=uu;
	m[1]=vv;
	m[2]=ww;
	return m;
}
void setCamera(vec2 fragCoord,out vec3 ro,out vec3 rd){
	vec2 mo=iMouse.xy+0.5;
	float theta=PI*(mo.y-0.5);
	float phi=2.*PI*mo.x-PI/2.;
	//eye parameters
	float ans=0;//.*iTime;

	ro=0.1*(30)*vec3(cos(phi)*cos(theta),sin(theta),sin(phi)*cos(theta));
	//ro=CAMERA_DIST*vec3(cos(phi)*cos(theta),sin(theta),sin(phi)*cos(theta));
	vec3 up=vec3(0,1,0);
	vec3 center=vec3(0,0,0);	//look at point
	
	//eye matrix
	mat3 V=lookAt(ro,center,up);
	//cast a view ray
	rd=normalize(V*vec3(fragCoord,-1.));	//world space
}
void setCameraOrtho(vec2 fragCoord, out vec3 ro,out vec3 rd)
{
	vec2 mo = vec2(0.5);
	//if(mouseButtonAction[0]==1)
	//	mo = mousePos;
	mo+=iMouse.xy;
	float theta=PI*(mo.y-0.5);
	float phi=2.*PI*mo.x-PI/2.;

	float ans=0;//.*iTime;
	ro=CAMERA_DIST*vec3(cos(phi)*cos(theta),sin(theta),sin(phi)*cos(theta));
	//ro=vec3(0,0,c);

	rd=normalize(-ro);

	vec3 up=vec3(0,1,0);
	vec3 center=vec3(0,0,0);	//look at point
	mat3 V=lookAt(ro,center,up);
	ro+=V*vec3(fragCoord,0);
}

//===============================================
// Obj intersectSDF(in Obj A, in Obj B) {
//     if(A.depth>0.&&A.depth>B.depth)return A;
//     return B;
// }

Obj unionSDF(Obj A, Obj B) {
	if(A.depth>0.&&B.depth>0.){
		if(A.depth<B.depth)return A;
		return B;
	}
	if(A.depth<0.&&B.depth<0.)return DEFAULT_OBJ;
	if(A.depth>=0.)return A;
	return B;
}

float planeSDF(vec3 ro,vec3 rd,vec3 n){
	return (0.-dot(ro,n))/dot(rd,n);
}
float sphereSDF(vec3 ro,vec3 rd,float R){
	float h;
	//ray intersection
	vec3 ce=ro;
	float a=dot(rd,rd);
	float b=dot(ce,rd);
	float c=dot(ce,ce)-R*R;
	h=b*b-a*c;
	if(h>0.)
	{
		h=(-b-sqrt(h));
		return h/a;
	}
	return RAY_MAXDEPTH;
}

Obj sceneSDF(vec3 ro,vec3 rd){
	Obj res=Obj(RAY_MAXDEPTH,-1);
	res=unionSDF(res,Obj(planeSDF(ro-groundOffset,rd,vec3(0.,1.,0.)),0));
	res=unionSDF(res,Obj(sphereSDF(ro-sc0,rd,1.),1));
	return res;
}

void getObjectAttributeById(int id,vec3 p,out ObjAttribute objAttribute){
	switch(id){
		case 0://ground
		{
			objAttribute.N=vec3(0,1,0);
 			//objAttribute.texCoord;
 			objAttribute.albedo=vec3(1.);
 			objAttribute.material=Material(vec3(0.),vec3(0.),vec3(0.),vec3(0.),0.);
			objAttribute.materialPbr=MaterialPBR(.0f,0.f,1.f);
			return;
		}
		case 1://sphere
		{
			objAttribute.N=normalize(p-sc0);
 			objAttribute.texCoord=vec2(atan(objAttribute.N.z,-objAttribute.N.x)/(2.*PI)+0.5,acos(objAttribute.N.y)/PI);
 			objAttribute.albedo=vec3(1.);
 			objAttribute.material=Material(vec3(0.),vec3(0.),vec3(0.),vec3(0.),0.);
			objAttribute.materialPbr=MaterialPBR(.0f,0.f,1.f);
			return;
		}
		default:
		{
			objAttribute.N=vec3(0,1,0);
 			objAttribute.texCoord=vec2(0);
 			objAttribute.albedo=vec3(1.);
 			objAttribute.material=Material(vec3(0.),vec3(0.),vec3(0.),vec3(0.),0.);
			objAttribute.materialPbr=MaterialPBR(1.f,0.f,1.f);
		}
	}
}

vec3 getBackgroundColor(vec3 rd)
{
	//return vec3(vec2(atan(rd.z,-1)/(2*PI)+0.5,acos(rd.y)/PI),0);
	return textureLod(textures[1],vec2(atan(rd.z,rd.x)/(2*PI)+0.5,acos(rd.y)/PI),0).rgb;
}

//=====================================================
//rendering
float diff_lambertian()
{
	return 1./PI;
}
vec3 Fresnel0(vec3 albedo,float metallic)
{
	return mix(vec3(0.04),albedo,metallic);
}
vec3 Fresnel(vec3 F0,float NdotL)
{
	return mix(vec3(pow(2.,(-5.355473*NdotL-6.98316)*NdotL)),vec3(1.),F0);
}
vec3 diff_basic(vec3 F)
{
	return (1.-F)*diff_lambertian();
}

void main()
{
	vec2 uv=getUV();
	//fragColor=vec4(uv,0,1);

	//step 1
	vec3 ro,rd;
	setCamera(uv,ro,rd);
	//setCameraOrtho(uv*CAMERA_DIST,ro,rd);
	
   	//vec3 col=vec3(ro.xyz);
   	vec3 col=getBackgroundColor(rd);

	//step 2
	Obj interP=sceneSDF(ro,rd);
	//col=vec3(interP.depth)/1000.;
	if(interP.depth<RAY_MAXDEPTH){
		

		vec3 pos=ro+interP.depth*rd;
		ObjAttribute objAttribute;
		getObjectAttributeById(interP.id,pos,objAttribute);
		vec3 L=normalize(-directionalLight.direction);
		float NdotL=max(dot(objAttribute.N,L),0.);

		vec3 fd=vec3(0);
		switch(0){
			case 0:
				//lambertian diffuse factor
				fd=vec3(diff_lambertian());
			break;
			case 1:
				//diffuse factor with simple balance, metall diffuse is small
				vec3 F0=Fresnel0(objAttribute.albedo,objAttribute.materialPbr.metallic);
				vec3 F=Fresnel(F0,NdotL);
				fd=vec3(diff_basic(F));
			break;
		}
		//vec3 albedo=texture(textures[0],objAttribute.texCoord).rgb;
		//col=vec3(objAttribute.texCoord,0);
		//col=albedo;
		col=PI*objAttribute.albedo*fd*NdotL*directionalLight.intensity;
		//col=fd;
		//col=objAttribute.N;
	}
	//fragColor=col.x;
	fragColor=vec4(col,1);
}