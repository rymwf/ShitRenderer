#version 460
layout(location=0) out vec4 fragOut;
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

/////////////////////////////////////////////

#define ZERO 0
#define PI 3.14159265358979323846
#define DEG2RAD(a) a*PI/180.

float maxcomp(vec2 v){
	return max(v.x,v.y);
}

float maxcomp(vec3 v){
	return max(max(v.x,v.y),v.z);
}
float maxcomp(vec4 v){
	return max(max(max(v.x,v.y),v.z),v.w);
}

mat3 identity(inout mat3 m){
    m[0]=vec3(0.);
    m[1]=vec3(0.);
    m[2]=vec3(0.);
    m[0][0]=1.;
    m[1][1]=1.;
    m[2][2]=1.;
    return m;
}
//rotate with x y z
mat3 rotate(float angle, vec3 v){
    float a = DEG2RAD(angle);
	float c = cos(a);
	float s = sin(a);

    vec3 axis=normalize(v);
    vec3 temp=(1. - c) * axis;

    mat3 Rotate;
    Rotate[0][0] = c + temp[0] * axis[0];
	Rotate[0][1] = temp[0] * axis[1] + s * axis[2];
	Rotate[0][2] = temp[0] * axis[2] - s * axis[1];

	Rotate[1][0] = temp[1] * axis[0] - s * axis[2];
	Rotate[1][1] = c + temp[1] * axis[1];
	Rotate[1][2] = temp[1] * axis[2] + s * axis[0];

	Rotate[2][0] = temp[2] * axis[0] + s * axis[1];
	Rotate[2][1] = temp[2] * axis[1] - s * axis[0];
	Rotate[2][2] = c + temp[2] * axis[2];
	return Rotate;
}


mat3 scale(vec3 factor){
    mat3 m;
    identity(m);
    m[0][0]=factor[0];
    m[1][1]=factor[1];
    m[2][2]=factor[2];
    return m;
}

//eye matrix
mat3 lookAt(vec3 pos,vec3 lookat,vec3 up){
    mat3 m;
    identity(m);
    
    //eye matrix
    vec3 ww=-normalize(lookat-pos);
    vec3 uu=normalize(cross(up,ww));
    vec3 vv=normalize(cross(ww,uu));
    m[0]=uu;
    m[1]=vv;
    m[2]=ww;
    return m;    
}

mat3 surfaceTBN(vec3 N){
    mat3 m;
    identity(m);

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    mat3 TBN = mat3(tangent, bitangent, N);
    return TBN;
}


float hash11( float n )    // in [0,1]
{
    return fract(sin(n*13.)*43758.5453);
}
float hash21 (in vec2 _st) {
    return fract(sin(dot(_st.xy,vec2(12.9898,78.233)))*43758.5453123);
}

vec2 hash22( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

float hash31(vec3 p)  // replace this by something better
{
    #if 0
    p  = 13.0*fract( p*0.3183099 + .1);
    return -1.0 + 2.0*fract( p.x*p.y*p.z*(p.x+p.y+p.z) );
    #else
    float n = p.x + p.y*57.0 + 113.0*p.z;	//?????
    return hash11(n);
    #endif
}
//value noise
// 2D Noise based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise21(in vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    // Four corners in 2D of a tile
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));

    // Smooth Interpolation

    // Cubic Hermine Curve.  Same as SmoothStep()
    vec2 u = f*f*(3.0-2.0*f);
    // u = smoothstep(0.,1.,f);

    // Mix 4 coorners percentages
    //equal to  mix(mix(a,b,u.x),mix(c,d,u.x),u.y)
    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}
//fbm fractal Brownian Motion
float fbm(in vec2 st, int octaves) {
    // Initial values
    float value = 0.0;
    float amplitude = .5;
    float frequency = 0.;
    //
    // Loop of octaves
    for(int i = 0; i < octaves; i++){
        value += amplitude * noise21(st);
        st *= 2.;
        amplitude *= .5;
    }
    return value;
}

//3d noise
// return value noise (in x) and its derivatives (in yzw)
float noise31( in vec3 x )
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0-2.0*f);
    return mix(mix(mix( hash31(i+vec3(0,0,0)), 
                        hash31(i+vec3(1,0,0)),f.x),
                   mix( hash31(i+vec3(0,1,0)), 
                        hash31(i+vec3(1,1,0)),f.x),f.y),
               mix(mix( hash31(i+vec3(0,0,1)), 
                        hash31(i+vec3(1,0,1)),f.x),
                   mix( hash31(i+vec3(0,1,1)), 
                        hash31(i+vec3(1,1,1)),f.x),f.y),f.z);
}

//fbm fractal Brownian Motion
float fbm(in vec3 st, int octaves) {
    // Initial values
    float value = 0.0;
    float amplitude = .5;
    float frequency = 0.;
    //
    // Loop of octaves
    for (int i = 0; i < octaves; i++) {
        value += amplitude * noise31(st);
        st *= 2.;
        amplitude *= .5;
    }
    return value;
}


// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float radicalInverse_VdC(uint bits) {
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
 }

 vec2 hammersley2d(uint i, uint N) {
     return vec2(float(i)/float(N), radicalInverse_VdC(i));
 }





float DiffuseOrenNayar(float roughness,vec3 L,vec3 V,vec3 N){
    float res=0.;
    float roughness2=roughness*roughness;
    float A=1.-.5*roughness2/(roughness2+.33);
    float B=.45*roughness2/(roughness2+.09);

    float VdotN=max(dot(V,N),0.);
    float aVN=acos(VdotN);

    float LdotN=max(dot(L,N),0.);
    float aLN=acos(LdotN);
    float alpha=max(aVN,aLN);
    float beta=min(aVN,aLN);
    float c=max(0.,cos(aLN-aVN));
    res=LdotN*(A+B*c*sin(alpha)*sin(beta))/PI;     //divide by PI
    return max(res,0.);
}
float DiffuseLamber(vec3 L,vec3 N){
	float res=max(0.,dot(N,L));    
    return (res);
}
float SpecularBlinnPhong(float shinness,vec3 L,vec3 V,vec3 N){
    //shadow
	float res=(0.);
    vec3 H=normalize(L+V);
    res=pow(max(0.,dot(N,H)),shinness);    
    return res;
}

vec3 F0Unreal(float ior,vec3 albedo,float metallic){
    float F0 = abs ((1.0 - ior) / (1.0 + ior));
    F0 = F0 * F0;
    return mix(vec3(F0), albedo, metallic);
}

float F0Schlick(float n1,float n2){
    return pow((n1-n2)/(n1+n2),2.);
}

vec3 F_Schlick(vec3 F0,float VdotN){
    // return R0+(1.-R0)*pow(1.-cosTheta,5.);
    return F0 + (1.0 - F0) * exp2((-5.55473 * VdotN - 6.98316) * VdotN);
}

vec3 F_SchlickRoughness(vec3 F0,float VdotN,float roughness){
    // return R0+(1.-R0)*pow(1.-cosTheta,5.);
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * exp2((-5.55473 * VdotN - 6.98316) * VdotN);
}

float G_SchlickBeckmann(float k,vec3 i,vec3 H){
    float cosTheta=max(0.,dot(i,H));
    return cosTheta/(cosTheta*(1.-k)+k);
}


float Gp_GGX(float alpha2,vec3 i,vec3 N,vec3 H){
	//alpha is roughness
    float idotH2=dot(i,H);
    float chi=step(0.,idotH2/dot(i,N));
    idotH2=idotH2*idotH2;
    float tan2=(1.-idotH2)/idotH2;
    return chi*2./(1.+sqrt(1.+alpha2*tan2));
}

float G_GGX(float alpha2,vec3 L,vec3 V,vec3 N,vec3 H){
    return Gp_GGX(alpha2,L,N,H)*Gp_GGX(alpha2,V,N,H);
}


float G_Smith(float k,vec3 L,vec3 V,vec3 N){
    return G_SchlickBeckmann(k,L,N)*G_SchlickBeckmann(k,V,N);
}

float D_GGX(float alpha2,vec3 H,vec3 N){
    float cosTheta=max(0.,dot(H,N));
    float cosTheta2=cosTheta*cosTheta;
    float temp=1.+cosTheta2*(alpha2-1.);
    return alpha2/(PI*temp*temp);
}


vec3 ImportanceSampleGGX(vec2 Xi, float roughness)
{
    float a = roughness*roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    return H;
}

const int SAMPLENUM=16;
vec3 SpecularCookTorrance(vec3 R0,float roughness,vec3 ro,vec3 L, vec3 V,vec3 N,out vec3 ks){
	
    vec3 col=vec3(0.);
    //V is vector to eye
    //float alpha=roughness*roughness;
    float alpha=roughness;
    float alpha2=alpha*alpha;
    float VdotN=max(dot(V,N),0.);
    if(true){
    //if(interP.depth>=RAY_MAXDEPTH){
        vec3 H=normalize(L+V);
        float HdotN=max(dot(H,N),0.);
       	
        float VdotH=max(dot(V,H),0.);
    	//vec3 F=F_Schlick(R0,VdotH);
        vec3 F=F_SchlickRoughness(R0,VdotN,roughness);
        float D=D_GGX(alpha2,H,N);
        //float G=G_Smith(k,rd[i],V,N);
        float G=G_GGX(alpha2,L,V,H,N);
        float cosTheta=max(0.,dot(L,N));
        ks+=F;
       	col+=F*D*G/max(4.*VdotN,0.5);
    }
    
    return col;
}


/////////////////////////////////////////////
//precision highp float;

#define RAY_MAXDEPTH 30.
#define RAY_INTERVAL 0.001
#define RAY_NUMSTEPS 100
#define EPSILON 0.00001

#define IOR_AIR 1.0 
#define IOR_WATER 1.33 
#define IOR_GLASS 1.5
#define IOR_DIAMOND 2.4

struct Light{
	vec3 pos;
    vec3 intensity;
};

struct Obj{
	float depth;
    int id;
};

struct Material{
    vec3 albedo;
    float roughness;	//roughness [0 1] 1 for rough surface
    float metallic; //[0 1] 1 metal specular coefficient
    float ior;
};

const int lightNum=1;
const Light lights[]=Light[lightNum](Light(vec3(1.),vec3(1.)));
const vec3 ambient=vec3(0.02);

const vec3 albedoGold=vec3(255,215,0)/255.;
const vec3 albedoDarkGold=vec3(184,134,11)/255.;

Material getMaterialById(int id){
    switch(id){
        case 0: //ground
        return Material(vec3(0.8,0.9,0.6)*0.1,.1,0.,1.5);
        case 1: //
        return Material(vec3(1.),.5,0.,IOR_WATER);
        case 3: //glass
        return Material(vec3(0.),.1,0.,IOR_GLASS);

        
        case 11:
        return Material(albedoDarkGold,.01,1.,0.27732);  
        case 12:
        return Material(albedoDarkGold,.2,1.,0.27732);  
        case 13:
        return Material(albedoDarkGold,.5,1.,0.27732);  
        case 14:
        return Material(albedoDarkGold,.8,1.,0.27732);  
        case 15:
        return Material(albedoDarkGold,1.,1.,0.27732);  

        case 21:
        return Material(albedoDarkGold,.01,0.,1.5);  
        case 22:
        return Material(albedoDarkGold,.2,0.,1.5);  
        case 23:
        return Material(albedoDarkGold,.5,0.,1.5);  
        case 24:
        return Material(albedoDarkGold,.8,0.,1.5);  
        case 25:
        return Material(albedoDarkGold,1.,0.,1.5);  

    }
    //no intersection
    return Material(vec3(0.),0.,1.,1.);
}

float checkersMod( in vec3 p )
{
    vec3 q = floor(p);
    return clamp(mod(q.x+q.y+q.z,2.),0.,1.);
}

Obj intersectSDF(in Obj A, in Obj B) {
    if(A.depth>B.depth)return A;
    return B;
}

Obj unionSDF(Obj A, Obj B) {
    if(A.depth<B.depth)return A;
    return B;
}


Obj differenceSDF(Obj A, Obj B) {
    B.depth*=-1.;
    if(A.depth>B.depth)return A;
    return B;
}


float intersectSDF(float A, float B) {
    return A>B?A:B;
}

float unionSDF(float A, float B) {
    return A<B?A:B;
}

float differenceSDF(float A, float B) {
    B*=-1.;
    return A>B?A:B;
}

float sphereSDF(vec3 pos,float R){
	//base sphere R=1, center=0;
    return (length(pos)-R);
}
float cubeSDF(vec3 p,vec3 size, float r){
    vec3 d = abs(p)-size+r;
    return length(max(d,0.0))+min(maxcomp(d),0.0)-r;
}

float infinitePlaneSDF(vec3 p,vec3 n){
	n=normalize(n);
    return dot(p,n);
}

float groundHeight(vec2 xz){
    return fbm(xz,3);
	//return sin(xz.x)*sin(xz.y);
}

//setup scene
Obj sceneSDF(vec3 p){
    Obj res=Obj(RAY_MAXDEPTH,-1);
    //res=unionSDF(res,Obj(sphereSDF(p-vec3(0.,1.9,0.),1.),3));
    
    for(int i=-1;i<1;++i){
        for(int j=-1;j<1;++j){
            for(int k=-1;k<1;++k){
            	res=unionSDF(res,Obj(cubeSDF(p-(vec3(i,j,k)+.5)*3.*abs(sin(iTime*.2)),vec3(1.0),1.-abs(sin(iTime*.4))),3));
            }
        }
    }
    
    return res;
}

vec3 envColor(vec3 rd,float LodLevel){
    //return vec3(0.);
    //how to calc lod level??
	return textureLod(textures[1],vec2(atan(rd.z,rd.x)/(2*PI)+.5,acos(rd.y)/PI),0).rgb;

 //   vec3 colSharp=pow(texture(iChannel0,rd).rgb,vec3(2.2));
 //   vec3 colBlur=pow(texture(iChannel1,rd).rgb,vec3(1.0));
	//return mix(colSharp,colBlur,LodLevel);
}


float calcSoftShadow(vec3 ro, vec3 rd,float tmin,float tmax,float k){
	float res=1.;
    Obj temp;
    for(float t=tmin;t<tmax;){
        vec3 pos=ro+t*rd;
        temp=sceneSDF(pos);
        if(abs(temp.depth)<RAY_INTERVAL)return 0.;
        res=min(res,k*temp.depth/t);
        t+=temp.depth;
    }
    return res;
}

Obj rayMarch(vec3 ro,vec3 rd){
    const float tmax=RAY_MAXDEPTH;
    const float tmin=RAY_INTERVAL+0.01;
    Obj res=Obj(tmax,-1);	//no intersection
    
    //ground
#if 0
    res=castTerrianRay(ro,rd);
    ////float groundHeight=hash11(iTime);
    //float d=(-0.1-ro.y)/rd.y;
    //if(d>tmin&&d<tmax){
    //    vec3 pos=ro+d*rd;
    //    //d+=fbm(pos+iTime,5);
    //    res=Obj(d,0);
    //}
#endif
    //scene shapes
    Obj temp;
    float t=tmin;
    for(int i=0;i<RAY_NUMSTEPS;++i){
        vec3 pos=ro+t*rd;
        temp=sceneSDF(pos);
        t+=abs(temp.depth);
        if(abs(temp.depth)<RAY_INTERVAL||t>tmax)break;
    }
    temp.depth=t;
    res=unionSDF(res,temp);
    return res;
}

// https://iquilezles.org/articles/normalsSDF
vec3 calcNormal( in vec3 pos )
{
#if 0
    //vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;
    //return normalize( e.xyy*sceneSDF( pos + e.xyy ).depth + 
	//				  e.yyx*sceneSDF( pos + e.yyx ).depth + 
	//				  e.yxy*sceneSDF( pos + e.yxy ).depth + 
	//				  e.xxx*sceneSDF( pos + e.xxx ).depth );
    vec3 eps = vec3(RAY_INTERVAL,0.0,0.0);
	return normalize( vec3(
           sceneSDF(pos+eps.xyy).depth - sceneSDF(pos-eps.xyy).depth ,
           sceneSDF(pos+eps.yxy).depth  - sceneSDF(pos-eps.yxy).depth ,
           sceneSDF(pos+eps.yyx).depth  - sceneSDF(pos-eps.yyx).depth  ) );
#else
    // inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
    vec3 n = vec3(0.0);
    for( int i=ZERO; i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*sceneSDF(pos+0.0005*e).depth;
    }
    return normalize(n);
#endif    
}



vec3 shading(in vec3 R0,in Light lights[lightNum],in Material material,vec3 pos,vec3 V,vec3 N){
    vec3 col=vec3(0.);

	vec3 kd=vec3(1.);
    
    for(int i=0;i<lightNum;++i){
        #if 0
        vec3 tempL=lights[i].pos-pos;
        vec3 L=normalize(tempL);
        vec3 intensity=lights[i].intensity/pow(length(tempL),2.);
        #else
        vec3 L=normalize(lights[i].pos);
        vec3 intensity=lights[i].intensity; 	//no attenuation
        #endif

        //float k_shadow=calcShadow(pos,L,0.001,length(tempL));	//sharp shadow
        //float k_shadow=calcSoftShadow(pos,L,RAY_INTERVAL+0.0001,RAY_MAXDEPTH,16.);	//soft shadow
        float k_shadow=1.;
        #if 0
        float d=DiffuseLamber(L,N);
        #else
        float d=DiffuseOrenNayar(material.roughness,L,V,N)*PI;
        #endif

        #if 0
        float shinness=max(pow(1.-material.roughness,4.)*500.,10.); //replace this
        float s=SpecularBlinnPhong(shinness,L,V,N)*(1.-material.roughness);
        kd=vec3(material.roughness);
		#else
        
        vec3 ks=vec3(0.);
        vec3 s=SpecularCookTorrance(R0,material.roughness,pos,L,V,N,ks);
        kd=1.-ks;
        #endif
        
        col+=k_shadow*intensity*(material.albedo*(1.-material.metallic)*kd*d+s);
        //col+=k_shadow*intensity*(material.albedo*(1.-material.metallic)*kd*d);
        //col+=k_shadow*intensity*(s);
        //col=vec3(s);
    }
    col+=ambient*material.albedo;
    return col;
}



struct Para{
    vec3 ro;
    vec3 rd;
    float ior;
    vec3 attenuation;
};
const int STACKCAPACITY=10;
const int MAXITERATION=40;
const vec3 a_glass=vec3(0.2,.5,.2);
const vec3 a_water=vec3(.3,.3,.2); 

vec3 render(vec3 ro,vec3 rd){
    // float curIOR=IOR_AIR,nextIOR=IOR_AIR;

    vec3 col=vec3(0.);
    Para paras[STACKCAPACITY];
    paras[0]=Para(ro,rd,IOR_AIR,vec3(1.0));
    int count=0;
    for(int i=0;i>-1&&i<STACKCAPACITY&&count<MAXITERATION;--i,++count){
        Para curPara=paras[i];
        if(all(lessThan(curPara.attenuation,vec3(.1)))){continue;};
        //vec3 color=curPara.attenuation*bgColor(curPara.rd);

        Obj interP=rayMarch(curPara.ro,curPara.rd);	//intersection pos depth
        if(interP.depth<.01){continue;}
        if(interP.id!=-1){
            //col=bgColor(curPara.rd);
            vec3 pos=curPara.ro+interP.depth*curPara.rd;
            Material material=getMaterialById(interP.id);
            vec3 V=normalize(curPara.ro-pos);
            vec3 N=vec3(0.);
            //if(interP.id==0)N=getTerrianNormal(pos);
            N=calcNormal(pos);
                
            vec3 R0=F0Unreal(material.ior,material.albedo,material.metallic);
            float VdotN=dot(V,N);

            if(true){
                vec3 F=vec3(0.0);
                if(VdotN<0.){N=-N,VdotN=-VdotN;}
                //F=F_Schlick(R0,VdotN);
                F=F_SchlickRoughness(R0,VdotN,material.roughness);
                #if 1 //refract
                if(interP.id==3){
                    vec3 k=vec3(1.);
                    //transparent object
                    float nextIOR=material.ior;
                    if(curPara.ior==nextIOR){
                        nextIOR=IOR_AIR;
                        k=exp(-a_glass*interP.depth);
                    }
                    vec3 t=refract(-V,N,curPara.ior/nextIOR);
                    if(t!=vec3(0.)){
                        //push refract
                        paras[i]=Para(pos,t,nextIOR,curPara.attenuation*k*(1.-F));
                        ++i;
                    }else F=vec3(1.0);
                }else if(interP.id==0){
					col+=vec3(0.0,0.09,0.18);   //sea base color                 
                }
                #endif

                #if 1 //enable reflect cannot use width IBL
                    vec3 r=reflect(-V,N);
                    float b=pow((1.-material.roughness),1.);
                    paras[i]=Para(pos,r,curPara.ior,curPara.attenuation*F*b); //replace this
                    ++i;
                #endif
            }
            //if(interP.id==0){material.albedo=vec3(checkersMod(pos));}
			#if 1            
            col+=curPara.attenuation*shading(R0,lights,material,pos,V,N);
            #else 
            col+=shadingIBL(R0,material,pos,V,N);
        	#endif
        }else{
            col+=curPara.attenuation*envColor(curPara.rd,1.-curPara.attenuation.x);	//edit this
        }
    }
    return col;    
}

void setCamera(vec2 uv,out vec3 ro,out mat3 V){
    //
    vec2 mo;
    mo.x= iMouse.x*PI*2.;
    mo.y= (iMouse.y-.5)*PI;
    float ans=0.2*iTime; 
    const float r=6.;    
    ro = vec3( r*cos(ans + 6.0*mo.x), 0.0 - 3.0*mo.y, r*sin(ans + 6.0*mo.x) );
    //ro=vec3(r*cos(mo.x)*cos(mo.y),0.+r*sin(-mo.y),r*sin(mo.x)*cos(mo.y));
    
    vec3 eyeUp=vec3(0,1,0);
    vec3 eyeLookat=vec3(0,0,0);	//look at point
    
    //eye matrix
    V=lookAt(ro,eyeLookat,eyeUp);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    
    mat3 V;
    vec3 ro,rd;
    //
    setCamera(uv,ro,V);
    vec3 col=vec3(0.);

    #define AA 2
    for(int i=0;i<AA;++i){
        for(int j=0;j<AA;++j){
            //cast a view ray
            vec2 p=(fragCoord+(vec2(i,j)+.5)/float(AA))/iResolution.xy-0.5;
            //p.y*=float(iResolution.y)/iResolution.x;
            p.x*=iResolution.x/iResolution.y;
            rd=normalize(V*vec3(p.x,p.y,-1.));	//world space
            col+=render(ro,rd);
        }
    }
    //col=rendering(ro,rd);
    col/=float(AA*AA);

    //float exposure=1.2;
    //col = vec3(1.0) - exp((-col) * exposure);

    col=pow(col,vec3(.4545));

    fragColor=vec4(col,1);
}

void main()
{
    vec2 uv=gl_FragCoord.xy;
    #ifdef VULKAN
    uv.y=iResolution.y-gl_FragCoord.y;
    #endif
    mainImage(fragOut,uv);
}