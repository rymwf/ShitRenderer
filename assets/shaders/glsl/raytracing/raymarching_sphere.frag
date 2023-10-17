#version 460
layout(location=0)out vec4 fragColor;
layout(binding=0)uniform UBO
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

layout(binding=1)uniform sampler2D textures[4];

//==============

#define RAYMARCH_MAXDEPTH 20.
//#define RAYMARCH_INTERVAL 0.001
#define RAYMARCH_NUMSTEPS 100
#define EPSILON .00001
#define PI 3.141592653
#define CAMERA_DIST 2.3

#define ZERO 0

const vec3 sc0=vec3(0.);//sphere center
const vec3 groundOffset=vec3(0.,-1,0.);//
const vec3 ambientLight=vec3(.2);

struct Light{
	vec3 direction;
	vec3 intensity;
};

Light directionalLight=Light(vec3(-1,-1,0),vec3(1));

struct Obj{
	float depth;
	int id;
};
struct Material{
	vec3 kambiendt;//ambient
	vec3 kd;//diffuse
	vec3 ks;//specular
	vec3 km;
	float shinness;//shinees
};
struct MaterialPBR{
	float metallic;
	float roughness;
	float ambientOcclusion;
};struct ObjAttribute
{
	//vec3 N;
	vec2 texCoord;
	vec3 albedo;
	Material material;
	MaterialPBR materialPbr;
};

vec2 getUV()
{
	vec2 uv=(gl_FragCoord.xy+vec2(.5))/iResolution.xy;
	uv-=.5;
	#ifdef VULKAN
	uv.y*=-1;
	#endif
	if(iResolution.x>iResolution.y)
	uv.x*=iResolution.x/iResolution.y;
	else
	uv.y*=iResolution.y/iResolution.x;
	return uv;
}

float maxcomp(vec2 v){
	return max(v.x,v.y);
}

float maxcomp(vec3 v){
	return max(max(v.x,v.y),v.z);
}
float maxcomp(vec4 v){
	return max(max(max(v.x,v.y),v.z),v.w);
}

mat3 surfaceTBN(vec3 N){
	vec3 UpVec=abs(N.z)<.999?vec3(0,0,1):vec3(1,0,0);
	//vec3 UpVec = abs(N.y) < 0.999 ? vec3(0,1,0):vec3(1,0,0);
	//vec3 UpVec=vec3(0,1,0);
	vec3 T=normalize(cross(UpVec,N));
	vec3 B=cross(N,T);
	return mat3(T,B,N);
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
	float phi=2.*PI*mo.x+PI/2.;

	//eye parameters
	float ans=0;//.*iTime;
	
	ro=.1*(50)*vec3(cos(phi)*cos(theta),sin(theta),sin(phi)*cos(theta));
	vec3 up=vec3(0,1,0);
	vec3 center=vec3(0,0,0);//look at point
	
	//eye matrix
	mat3 V=lookAt(ro,center,up);
	//cast a view ray
	rd=normalize(V*vec3(fragCoord,-1.));//world space
}
void setCameraOrtho(vec2 fragCoord,out vec3 ro,out vec3 rd)
{
	//vec2 mo=vec2(.5);
	//if(mouseButtonAction[0]==1)
	//mo=mousePos;
	vec2 mo=iMouse.xy;
	float theta=PI*(mo.y-.5);
	float phi=2.*PI*mo.x-PI/2.;
	
	float ans=0;//.*iTime;
	ro=CAMERA_DIST*vec3(cos(phi)*cos(theta),sin(theta),sin(phi)*cos(theta));
	//ro=vec3(0,0,c);
	
	rd=normalize(-ro);
	
	vec3 up=vec3(0,1,0);
	vec3 center=vec3(0,0,0);//look at point
	mat3 V=lookAt(ro,center,up);
	ro+=V*vec3(fragCoord,0);
}

float sphereSDF(vec3 pos,float R){
	//base sphere R=1, center=0;
	return(length(pos)-R);
}

float cubeSDF(vec3 p,vec3 size){
	vec3 d=abs(p)-size;
	//return length(max(d,0.0))+ min(maxcomp(d),0.);
	return min(length(max(d,0.)),maxcomp(d));
	//float res=maxcomp(d);
	//if(res>0)
	//	res=length(max(d,0.));
	//return res;
}

float infinitePlaneSDF(vec3 p,vec3 center,vec3 n){
	//n=normalize(n);
	return dot(p-center,n);
}

Obj intersectSDF(in Obj A,in Obj B){
	return A.depth>B.depth?A:B;
}

Obj unionSDF(Obj A,Obj B){
	return A.depth<B.depth?A:B;
}

Obj differenceSDF(Obj A,Obj B){
	B.depth*=-1.;
	return A.depth>B.depth?A:B;
}

float intersectSDF(float A,float B){
	return A>B?A:B;
}

float unionSDF(float A,float B){
	return A<B?A:B;
}

float differenceSDF(float A,float B){
	B*=-1.;
	return A>B?A:B;
}

Obj sceneSDF(vec3 pos){
	Obj res=Obj(RAYMARCH_MAXDEPTH,-1);
	//res=differenceSDF(res,Obj(sphereSDF(pos,0.9),1));
	res=unionSDF(res,Obj(sphereSDF(pos-vec3(1,0,0),1.),1));
	//res=intersectSDF(res,Obj(cubeSDF(pos,vec3(0.8)),2));
	res=unionSDF(res,Obj(cubeSDF(pos-vec3(-1,0,0),vec3(1.)),2));
	//res=unionSDF(res,Obj(infinitePlaneSDF(pos,vec3(0,-1,0),vec3(0.,1.,0.)),0));
	return res;
}

void getObjectAttributeById(int id,vec3 p,out ObjAttribute objAttribute){
	switch(id){
		case 0://ground
		{
			objAttribute.texCoord=vec2(0);
			objAttribute.albedo=vec3(1.);
			objAttribute.material=Material(vec3(0.),vec3(0.),vec3(0.),vec3(0.),0.);
			objAttribute.materialPbr=MaterialPBR(.0f,0.f,1.f);
			return;
		}
		case 1://sphere
		{
			//objAttribute.texCoord=vec2(atan(objAttribute.N.z,-objAttribute.N.x)/(2.*PI)+0.5,acos(objAttribute.N.y)/PI);
			objAttribute.texCoord=vec2(0);
			objAttribute.albedo=vec3(1.);
			objAttribute.material=Material(vec3(0.),vec3(0.),vec3(0.),vec3(0.),0.);
			objAttribute.materialPbr=MaterialPBR(.0f,0.f,1.f);
			return;
		}
		case 2://cube
		{
			objAttribute.texCoord=vec2(0);
			objAttribute.albedo=vec3(1.);
			objAttribute.material=Material(vec3(0.),vec3(0.),vec3(0.),vec3(0.),0.);
			objAttribute.materialPbr=MaterialPBR(.0f,0.f,1.f);
			return;
		}
		default:
		{
			objAttribute.texCoord=vec2(0);
			objAttribute.albedo=vec3(1.);
			objAttribute.material=Material(vec3(0.),vec3(0.),vec3(0.),vec3(0.),0.);
			objAttribute.materialPbr=MaterialPBR(1.f,0.f,1.f);
		}
	}
}
vec3 getBackgroundColor(vec3 rd)
{
	return textureLod(textures[1],vec2(atan(rd.z,rd.x)/(2*PI)+.5,acos(rd.y)/PI),0).rgb;
}
/**
* Using the gradient of the SDF, estimate the normal on the surface at point p.
*/
vec3 estimateNormal(vec3 p){
	return normalize(vec3(
		sceneSDF(vec3(p.x + EPSILON, p.y, p.z)).depth - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)).depth,
		sceneSDF(vec3(p.x, p.y + EPSILON, p.z)).depth - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)).depth,
		sceneSDF(vec3(p.x, p.y, p.z  + EPSILON)).depth- sceneSDF(vec3(p.x, p.y, p.z - EPSILON)).depth
	));
}
	
// http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
vec3 calcNormal( in vec3 pos )
{
	#if 0
	vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;
	return normalize( e.xyy*sceneSDF( pos + e.xyy ).depth +
	e.yyx*sceneSDF( pos + e.yyx ).depth+
	e.yxy*sceneSDF( pos + e.yxy ).depth+
	e.xxx*sceneSDF( pos + e.xxx ).depth);
	#else
	// inspired by klems - a way to prevent the compiler from inlining map() 4 times
	vec3 n = vec3(0.0);
	for( int i=ZERO; i<4; i++ )
	{
		vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
		n += e*sceneSDF(pos+0.0005*e).depth;
	}
	return normalize(n);
	#endif
}
/*
https://iquilezles.org/www/articles/rmshadows/rmshadows.htm
*/
float calcShadow(vec3 ro,vec3 rd,float tmin,float tmax){
	Obj temp;
	for(float t=tmin;t<tmax;){
		vec3 pos=ro+t*rd;
		temp=sceneSDF(pos);
		if(temp.depth<.0001)return 0.;
		t+=temp.depth;
	}
	return 1.;
}

//iq
float calcSoftShadow(vec3 ro,vec3 rd,float tmin,float tmax,float k){
	float res=1.;
	float h;
	for(float t=tmin;t<tmax;){
		h=sceneSDF(ro+t*rd).depth;
		if(h<.001*t)return 0.;
		res=min(res,k*h/t);
		t+=h;
	}
	return res;
}
float calcSoftShadow2(vec3 ro,vec3 rd,float tmin,float tmax,float k){
	float res=1.;
	float ph=1e20;
	float h;
	for(float t=tmin;t<tmax;){
		h=sceneSDF(ro+t*rd).depth;
		if(h<.01*t)return 0.;
		float y=h*h/(2.*ph);
		float d=sqrt(h*h-y*y);
		res=min(res,k*d/max(0,t-y));
		t+=ph=h;
		
	}
	return res;
}
float calcAO(vec3 ro,vec3 rd)
{
	const float k=3.;
	const float dt=.1;
	float d;
	float ao=0.;
	int i=0;
	float t;
	vec3 pos;
	for(;i<5;++i)
	{
		t=.01+dt*i;
		pos=ro+t*rd;
		d=min(sceneSDF(pos).depth,pos.y-groundOffset.y);
		ao+=(t-d)/pow(2,i);
	}
	return 1-clamp(k*ao,0,1);
}

//m is shape coordinate
Obj rayMarch(vec3 ro,vec3 rd){
	float tmax=RAYMARCH_MAXDEPTH;
	float tmin=.1;
	Obj res=Obj(tmax,-1);//no intersection
	
	//ground
	float d=(groundOffset.y-ro.y)/rd.y;
	if(d>tmin&&d<tmax)
	res=Obj(d,0);
	//return res;
	
	//scene shapes
	Obj temp;
	float t=tmin;
	vec3 pos;
	for(int i=0;i<RAYMARCH_NUMSTEPS;++i){
		pos=ro+t*rd;
		temp=sceneSDF(pos);
		t+=abs(temp.depth);
		if(abs(temp.depth)<.001*t)
		{
			temp.depth=t;
			return temp;
		}else if(t>tmax)
		break;
	}
	return res;
}
//=====================================================
//rendering
float diff_lambertian()
{
	return 1./PI;
}
vec3 Fresnel0(vec3 albedo,float metallic)
{
	return mix(vec3(.04),albedo,metallic);
}
vec3 Fresnel(vec3 F0,float NdotL)
{
	return mix(vec3(pow(2.,(-5.355473*NdotL-6.98316)*NdotL)),vec3(1.),F0);
}
vec3 diff_basic(vec3 F)
{
	return(1.-F)*diff_lambertian();
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
	//vec3 col=getBackgroundColor(rd);
	vec3 col=vec3(0);
	vec3 albedo=vec3(1);
	
	//step 2
	Obj interP=rayMarch(ro,rd);
	//Obj interP=rayMarchConstantStep(ro,rd);
	
	if(interP.depth<RAYMARCH_MAXDEPTH){
		vec3 pos=ro+interP.depth*rd;
		vec3 N=interP.id==0?vec3(0.,1.,0.):calcNormal(pos);
		vec3 L=normalize(-directionalLight.direction);
		float NdotL=max(dot(N,L),0.);
		//vec3 N=interP.id==0?vec3(0.,1.,0.):estimateNormal(pos);
		ObjAttribute objAttribute;
		getObjectAttributeById(interP.id,pos,objAttribute);
		
		//float shadow=calcShadow(pos,L,0.01,RAYMARCH_MAXDEPTH);
		float shadow=calcSoftShadow2(pos,L,.01,RAYMARCH_MAXDEPTH,5);
		float ao=calcAO(pos,N);
		
		//albedo=texture(textures[0],objAttribute.texCoord).rgb;
		float fd=diff_lambertian();
		
		//col=vec3(objAttribute.texCoord,0);
		//col=albedo;
		col=shadow*PI*objAttribute.albedo*fd*NdotL*directionalLight.intensity+ao*ambientLight*objAttribute.albedo;
		//col=pos;
		//col=N;
		//col=vec3(shadow);
		//col=vec3(ao);
	}
	fragColor=vec4(col,1);
}