#version 460
layout(location=0) out vec4 fragOut;

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

//=============================
#define P_COUNT 10

vec2 points1[P_COUNT];
float hash11( float n )
{
    return fract(sin(n*13.)*43758.5453);
}
vec2 hash12(float n){
    return fract(vec2(sin(n*17.),cos(n*13.))*43758.5453);
}

vec3 checkboard(vec2 uv){
	uv=floor(uv);
    return vec3(mod(uv.x+uv.y,2.))-0.8;
}

float lineSDF(vec2 p,vec2 p1,vec2 p2){
	vec2 v1=p-p1;
    vec2 v2=p2-p1;
    vec2 projv1ATv2=dot(v1,v2)*v2/dot(v2,v2);
    vec2 k=projv1ATv2/v2;
    
    if(k.x<0.||k.y<0.)return length(v1);
    if(k.x>1.||k.y>1.)return length(p-p2);
	float d=length(v1-projv1ATv2);
    return d;
}
float circleSDF(vec2 p,vec2 sc,float r){
	return length(p-sc)-r;
}

float ellipsoidSDF(vec2 p,vec2 sc,mat2 r){
    mat2 M=inverse(r);
    p=M*p;
    vec2 r2=vec2(length(r[0]),length(r[1]));
	return length((p-M*sc))-1.;
}

float unionSDF(float d1,float d2){return min(d1,d2);}

vec2 bmin=vec2(0.),bmax=vec2(0.);
float boxAABB(vec2 xy){
    float d=lineSDF(xy,bmin,vec2(bmax.x,bmin.y));
	d=unionSDF(d,lineSDF(xy,bmin,vec2(bmin.x,bmax.y)));
    d=unionSDF(d,lineSDF(xy,bmax,vec2(bmin.x,bmax.y)));
    d=unionSDF(d,lineSDF(xy,bmax,vec2(bmax.x,bmin.y)));
    return d;
}

mat2 axis=mat2(0.);
mat2 ext=mat2(0.);
vec2 sc=vec2(0.);
bool naturalAxis(vec2 pos[P_COUNT]){
	vec2 mid=vec2(0.);
    for(int i=0;i<P_COUNT;++i)mid+=pos[i];
	mid/=float(P_COUNT);
	mat2 M=mat2(0.);
    for(int i=0;i<P_COUNT;++i){
        vec2 v=pos[i]-mid;
    	M+=outerProduct(v,v);
   	}
    M/=float(P_COUNT);
    float b=-(M[0][0]+M[1][1])/2.;
    float c=M[0][0]*M[1][1]-M[0][1]*M[1][0];
    if(M[0][1]*M[1][0]==0.)return false;
    float delta=b*b-c;
    if(delta<=0.)return false;
    delta=sqrt(delta);
    vec2 lambda=vec2(delta,-delta)-b;
	mat2 M1=M-lambda[0]*mat2(1.,0.,0.,1.);
    
    axis[0]=normalize(vec2(1.,-M1[0][0]/M1[0][1]));
    M1=M-lambda[1]*mat2(1.,0.,0.,1.);
    axis[1]=normalize(vec2(1.,-M1[0][0]/M1[0][1]));
    ext[0]=vec2(dot(axis[0],pos[0]));
    ext[1]=vec2(dot(axis[1],pos[0]));
    for(int i=1;i<P_COUNT;++i){
    	vec2 aa=vec2(dot(axis[0],pos[i]),dot(axis[1],pos[i]));
        ext=mat2(min(ext[0].x,aa.x),
        max(ext[0].y,aa.x),
        min(ext[1].x,aa.y),
        max(ext[1].y,aa.y));
    }
    sc=((ext[0].x+ext[0].y)*axis[0]+(ext[1].x+ext[1].y)*axis[1])/2.;
    return true;
}


float boundingBox(vec2 xy){	
    mat4x2 pp;
    pp[0]=axis[0]*ext[0].x+axis[1]*ext[1].x;
    pp[1]=axis[0]*ext[0].y+axis[1]*ext[1].x;
    pp[2]=axis[0]*ext[0].y+axis[1]*ext[1].y;
    pp[3]=axis[0]*ext[0].x+axis[1]*ext[1].y;
    
    float d=lineSDF(xy,pp[0],pp[1]);
    d=unionSDF(d,lineSDF(xy,pp[1],pp[2]));
    d=unionSDF(d,lineSDF(xy,pp[2],pp[3]));
    d=unionSDF(d,lineSDF(xy,pp[3],pp[0]));
    
    #if 0
    vec2 lmin,lmax;
    lmin=pp[0];lmax=lmin;
    for(int i=1;i<4;++i){
    	lmin=min(pp[i],lmin);
        lmax=max(pp[i],lmax);
    }
    
    d=unionSDF(d,lineSDF(xy,lmin,vec2(lmax.x,lmin.y)));
    d=unionSDF(d,lineSDF(xy,lmin,vec2(lmin.x,lmax.y)));
    d=unionSDF(d,lineSDF(xy,lmax,vec2(lmin.x,lmax.y)));
    d=unionSDF(d,lineSDF(xy,lmax,vec2(lmax.x,lmin.y)));
    #endif
    
    return d;
}

float boundingSphere(vec2 xy,vec2 pos[P_COUNT]){
	vec2 pk,pl;
    pk=axis[0]*ext[0][0];
    pl=axis[0]*ext[0][1];
	float r=length((pk-pl)/2.);
    vec2 sc2=sc;
    for(int i=0;i<P_COUNT;++i){
        if(length(pos[i]-sc2)>r){
        	sc2=(pos[i]+sc2+normalize(sc2-pos[i])*r)/2.;
            r=length(sc2-pos[i]);
        }
    }
    float d=circleSDF(xy,sc2,r);
    return d;
}
float boundingEllipsoid(vec2 xy,vec2 pos[P_COUNT]){
    mat2 M=axis*mat2(1./(ext[0][1]-ext[0][0]),0.,0.,1./(ext[1][1]-ext[1][0]))*transpose(axis);
    for(int i=0;i<P_COUNT;++i)pos[i]=M*pos[i];
    float r=.5;
    vec2 sc2=M*sc;
    for(int i=0;i<P_COUNT;++i){
        if(length(pos[i]-sc2)>r){
        	sc2=(pos[i]+sc2+normalize(sc2-pos[i])*r)/2.;
            r=length(sc2-pos[i]);
        }
    }
    mat2 invM=inverse(M);
    sc2=invM*sc2;
    mat2 r2=invM*r;
    float d=min(ellipsoidSDF(xy,sc2,r2),1.);
    return d;
}

float pointSDF(vec2 uv,vec2 pos[P_COUNT]){
	float d=100.;
    for(int i=0;i<P_COUNT;++i){
    	d=unionSDF(d,length(uv-pos[i]));
    }
	return d;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    uv.y*=iResolution.y/iResolution.x;
    uv*=7.;
    uv.x-=1.5;
    uv.y-=1.0;
    
    for(int i=1;i<=P_COUNT;++i){
        float t=iTime*hash11(float(i))*3.;
    	int a=int(floor(t))+1;
        float b=fract(t);
    	points1[i-1]=mix(hash12(float(i*a)),hash12(float(i*(a+1))),b)*(hash12(float(i))-0.5)*3.+vec2(1.5,1.);
        if(i==1){bmin=points1[0];bmax=bmin;continue;}
        bmax=max(bmax,points1[i-1]);
        bmin=min(bmin,points1[i-1]);
    }
	if(!naturalAxis(points1))return;
    //
	vec3 bgColor=checkboard(uv);
    if(abs(uv.x)<.02||abs(uv.y)<.02)bgColor=vec3(0.0,0.3,.1);
    vec3 col=bgColor;
    
    //plot
    float f1=abs(boxAABB(uv));
   	col=mix(vec3(1.),col,smoothstep(0.01,0.02,f1));
    float f2=abs(boundingBox(uv));
   	col=mix(vec3(1.,0.,0.),col,smoothstep(0.01,0.02,f2));
    float f3=abs(boundingSphere(uv,points1));
   	col=mix(vec3(0.,1.0,0.),col,smoothstep(0.01,0.02,f3));
    float f4=abs(boundingEllipsoid(uv,points1));
   	col=mix(vec3(0.,.0,1.),col,smoothstep(0.01,0.02,f4));
    //show points
    float d=smoothstep(0.03,0.04,pointSDF(uv,points1));
    col=mix(vec3(1.0,0.7,0.0),col,d);
    
    // Output to screen
    fragColor = vec4(col,1.0);
}
void main()
{
    vec2 uv=gl_FragCoord.xy;
    #ifdef VULKAN
    uv.y=iResolution.y-gl_FragCoord.y;
    #endif
    mainImage(fragOut,uv);
}