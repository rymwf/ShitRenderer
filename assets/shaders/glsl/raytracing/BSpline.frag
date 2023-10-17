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

//=====================================
#define PI 3.141592653

/*
enable quadratic bezier spline
1 based on cubic polynomial
2 based on barycentric coordinate
*/
//#define BEZIERANALYTIC 2

#ifndef BEZIERANALYTIC
	/*
	0 cardinal cubic
	1 lagrange interpolation
	2 bezier spline
	3 Bspline quadratic
	4 Bspline cubic
	*/
	#define CURVETYPE 3	
	const int controlPointIndex=2;	
	#if 0
	const int P_COUNT=4;
	float Py[P_COUNT]=float[](0.,2.,1.,2.);
	float Px[P_COUNT]=float[](1.,1.,3.,3.);
	#else
	const int P_COUNT=7;
	float Py[P_COUNT]=float[](0.,1.,2.,1.,2.,1.,0.);
	float Px[P_COUNT]=float[](1.,0.,1.,2.,3.,4.,3.);
	#endif
	
#else
	const int controlPointIndex=1;
	const int P_COUNT=3;
	float Py[P_COUNT]=float[](0.,2.,0.);
	float Px[P_COUNT]=float[](1.,3.,3.);
#endif 


vec2 barycentricCoord(vec2 v,vec2 v1,vec2 v2){return inverse(mat2(v1,v2))*v;}

int fact(int n){
    int res=1;
    while(n>1){res*=n;--n;}
    return res;
}
int C(int n,int k){return fact(n)/(fact(n-k)*fact(k));}
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
float unionSDF(float d1,float d2){return min(d1,d2);}
float CardinalCubic(vec2 xy){
    //float k=3.*sin(iTime*2.);    //0 catmull rom spline
    float k=3.;    //0 catmull rom spline
	float s=(1.-k)/2.;    //scale factor
	mat4 B=mat4(0,1,0,0,
    		-s,0,s,0,
    		2.*s,s-3.,3.-2.*s,-s,
    		-s,2.-s,s-2.,s);
    B=transpose(B);
    int i=2;
    float d=100.;
    const int segments=20;
    vec2 a,b;
    a=vec2(Px[1],Py[1]);
    for(int i=2;i<P_COUNT-1;++i){     
    	vec4 py=vec4(Py[i-2],Py[i-1],Py[i],Py[i+1]);
    	vec4 px=vec4(Px[i-2],Px[i-1],Px[i],Px[i+1]);   	
    	vec4 ay=B*py;
    	vec4 ax=B*px;
    	for(int j=1;j<=segments;++j){
    		float t=float(j)/float(segments);
    	    vec4 u=vec4(1.,t,t*t,t*t*t);
    	    b=vec2(dot(u,ax),dot(u,ay));
            d=unionSDF(d,lineSDF(xy,a,b));
    		a=b;
        }
    }
    return d;
}

float LagrangeInterpolation(vec2 xy){
    const float interval=0.1;
    const int pointNum=int(float(P_COUNT-1)/interval)+1;
    vec2 A,B;
    A=vec2(Px[0],Py[0]);
    float d=100.;
    for(int k=1;k<pointNum;++k){
        B=vec2(0.);
        float t=float(k)*interval;
    	for(int i=0;i<P_COUNT;++i){
    	    float b=1.;
    	    for(int j=0;j<P_COUNT;++j){
    	    	if(i==j)continue;
    	    	b*=(t-float(j))/float(i-j);
    	    }
    	    B+=b*vec2(Px[i],Py[i]);
    	}
        d=unionSDF(d,lineSDF(xy,A,B));
        A=B;
    }
    return d;
}

float BezierSpline(vec2 xy){  
    float c[P_COUNT];
    int hn=P_COUNT/2;
    for(int i=0;i<=hn;++i){
    	c[i]=float(C(P_COUNT-1,i));
        c[P_COUNT-1-i]=c[i];
    }
    const int pointNum=40;
    vec2 A,B;
    A=vec2(Px[0],Py[0]);
    float d=length(xy-A);
    for(int k=1;k<=pointNum;++k){
    	float t=float(k)/float(pointNum);
        B=vec2(0.);
        for(int i=0;i<P_COUNT;++i){
        	B+=c[i]*pow(t,float(i))*pow(1.-t,float(P_COUNT-1-i))*vec2(Px[i],Py[i]);
        }
        d=unionSDF(d,lineSDF(xy,A,B));
        A=B;
    }
    return d;
}

vec3 cubicRoot(float a,float b,float c,float d){
	vec3 res;
    float p=(a*c-b*b/3.)/(a*a);
    float q=(2.*b*b*b/27.-a*b*c/3.+a*a*d)/(a*a*a);
    float der=(p*p*p/27.+q*q/4.);
    if(der<0.){
        //Trigonometric solution
        for(int i=0;i<3;++i){
        	res[i]=2.*sqrt(-p/3.)*cos(1./3.*acos(3.*q/(2.*p)*sqrt(-3./p))-float(i)*2.*3.141592653/3.)-b/(3.*a);
        }
    }else if(der>0.){
        //Cardano's method, only the real root, the other two complex roots can be computed by muliply (1+-sqrt(3))/2
        float temp=sqrt(der);
        vec2 z=-q/2.+vec2(temp,-temp);
    	z=sign(z)*pow(abs(z),vec2(1./3.));
        res=vec3(z.x+z.y-b/(3.*a)); 
    }else{
		res=vec3(1,-0.5,-0.5)*3.*q/p-b/(3.*a); 
    }
	return res;
}


float BezierSplineQuadAnalytic1(vec2 xy){
	float d=100.;
    vec2 A=vec2(Px[2],Py[2])-2.*vec2(Px[1],Py[1])+vec2(Px[0],Py[0]);
    vec2 B=(vec2(Px[1],Py[1])-vec2(Px[0],Py[0]));
	vec2 p=vec2(Px[0],Py[0])-xy;
    vec3 a=clamp(cubicRoot(dot(A,A),3.*dot(A,B),dot(A,p)+2.*dot(B,B),dot(B,p)),0.,1.);
    for(int i=0;i<3;++i){
    	d=unionSDF(d,length(a[i]*a[i]*A+2.*B*a[i]+p));
    }    
    return d;
}
float BezierSplineQuadAnalytic2(vec2 xy){
    vec2 p0=vec2(Px[0],Py[0]),p1=vec2(Px[1],Py[1]),p2=vec2(Px[2],Py[2]);
	vec2 v1=p1-p0,v2=p2-p0;
    vec2 uv=barycentricCoord(xy-p0,2.*v1,v2-2.*v1);
    return uv.x*uv.x-uv.y;
}


//uniform quad Bspline
float BsplineQuad(vec2 xy){
    float d=100.;
    const int kk=3;
    const int num0=10;
    const int num1=num0*kk+1;
    
    float b[num1];
    b[0]=0.;
    int hn=(num1+1)/2;
    for(int j=0;j<hn;++j){
        float t=float(j)/float(num0);
        float tt=fract(t);
        if(t<1.){
        	b[j]=tt*tt*0.5;
        }
        else if(t>=1.&&t<2.){
        	b[j]=-tt*tt+tt+0.5;
        }
        b[num1-1-j]=b[j];
    }
    vec2 A,B;
	for(int k=0;k<kk;++k){
    	A+=b[(kk-1-k)*num0]*vec2(Px[k],Py[k]);
    }
    for(int i=0;i<P_COUNT-kk+1;++i){
        for(int j=1;j<=num0;++j){
            B=vec2(0.);
            for(int k=0;k<kk;++k){
            	B+=b[(kk-1-k)*num0+j]*vec2(Px[k+i],Py[k+i]);
            }
            d=unionSDF(d,lineSDF(xy,A,B));
            A=B;
        }
    }
	return d;
}

//uniform
float BsplineCubic(vec2 xy){
	float d=100.; 
    const int kk=4;
    const int num0=10;
    const int num1=num0*kk+1;
    
    float b[num1];
    b[0]=0.;
    int hn=(num1+1)/2;
    for(int j=0;j<hn;++j){
        float t=float(j)/float(num0);
        float tt=fract(t);
        if(t<1.){
        	b[j]=tt*tt*tt/6.;
        }
        else if(t>=1.&&t<2.){
        	b[j]=(3.*tt*(tt*(1.-tt)+1.)+1.)/6.;
        }
        else if(t>=2.&&t<3.){
        	b[j]=(2.*tt*tt*(tt-2.)+4.)/6.;
        }
        b[num1-1-j]=b[j];
    }  
    vec2 A,B;
	for(int k=0;k<kk;++k){
    	A+=b[(kk-1-k)*num0]*vec2(Px[k],Py[k]);
    }
    for(int i=0;i<P_COUNT-kk+1;++i){
        for(int j=1;j<=num0;++j){
            B=vec2(0.);
            for(int k=0;k<kk;++k){
            	B+=b[(kk-1-k)*num0+j]*vec2(Px[k+i],Py[k+i]);
            }
            d=unionSDF(d,lineSDF(xy,A,B));
            A=B;
        }
    }
	return d;
}




float func(vec2 xy){
    float f=xy.y-xy.x;
    
    #ifdef BEZIERANALYTIC
    switch(BEZIERANALYTIC){
        case 1:
    	return BezierSplineQuadAnalytic1(xy);
        case 2:
        return BezierSplineQuadAnalytic2(xy);
    }
    #else
    switch(CURVETYPE){
    	case 0:
        f=CardinalCubic(xy);
        	break;
        case 1:
        f=LagrangeInterpolation(xy);
        	break;
        case 2:
        f=BezierSpline(xy);
        	break;
        case 3:
        f=BsplineQuad(xy);
        	break;
        case 4:
        f=BsplineCubic(xy);
        	break;
    }   
    #endif
    return f;
}

vec2 funcGrad(vec2 uv){
    vec2 h=vec2(0.01,0);
    return vec2(func(uv+h)-func(uv-h),func(uv+h.yx)-func(uv-h.yx))/(2.*h.x);
}

float pointSDF(vec2 uv){
	float d=100.;
    for(int i=0;i<P_COUNT;++i){
    	d=unionSDF(d,length(uv-vec2(Px[i],Py[i])));
    }
	return d;
} 
vec2 getUV()
{
	vec2 uv=(gl_FragCoord.xy+vec2(.5))/iResolution.xy;
	//uv-=.5;
	//#ifdef VULKAN
	//uv.y*=-1;
	//#endif
	//if(iResolution.x>iResolution.y)
	//	uv.x*=iResolution.x/iResolution.y;
	//else
		uv.y*=iResolution.y/iResolution.x;
	return uv;
}
void main()
{
    // Normalized pixel coordinates (from 0 to 1)
	//vec2 uv=getUV();
	vec2 uv=(gl_FragCoord.xy+vec2(.5))/iResolution.xy;
	#ifdef VULKAN
	uv.y=1-uv.y;
	#endif

	uv.y*=iResolution.y/iResolution.x;
    uv*=7.;
    uv.x-=1.5;
    uv.y-=1.0;
    
    vec2 mo=iMouse.xy;
    mo.y*=iResolution.y/iResolution.x;
    mo*=7.;
    mo.x-=1.5;
    mo.y-=1.0;
    mo.x-=Px[controlPointIndex];
    mo.y-=Py[controlPointIndex];
    

	vec3 bgColor=checkboard(uv);
    if(abs(uv.x)<.02||abs(uv.y)<.02)bgColor=vec3(0.0,0.3,.1);
    vec3 col=bgColor;
    
	Py[controlPointIndex]+=mix(sin(iTime)*1.5,mo.y,step(1.,iMouse.z));
    Px[controlPointIndex]+=mix(0.,mo.x,step(1.,iMouse.z));
    //plot
    float f=(func(uv))/length(funcGrad(uv));
    //float f=abs(func(uv));
    float e=0.01;
   	col=mix(vec3(0.9),col,smoothstep(0.01,0.02,f));
    f=sign(f)*clamp((sin(f*100.)*0.5-0.5)*(1.-f)+1.,0.,1.);
    if(f>0.)col=mix(vec3(0.4),col,f);
    else col=mix(vec3(0.8,0.5,0.),col,f);
    
    //show points
    float d=smoothstep(0.03,0.04,pointSDF(uv));
    col=mix(vec3(1.0,0.7,0.0),col,d);
    
    // Output to screen
    fragColor = vec4(col,1.0);
}