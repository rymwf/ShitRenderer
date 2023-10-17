#version 460
layout(location=0) out vec4 fragColor;
//layout(location=0) out float fragColor;

layout(binding=0) uniform UBO
{
	vec2 viewportSize;
	vec2 mousePos;//[0,1]
	vec2 mouseWheel;
	ivec3 mouseButtonAction;
};

layout(binding=1) uniform sampler2D textures[4];


//====================================
#define PI 3.14159265358979323846
#define RAY_MAXDEPTH 100.
#define CAMERA_DIST 2.3

vec2 getUV()
{
	vec2 uv=(gl_FragCoord.xy+vec2(0.5))/viewportSize;
	uv-=0.5;
#ifdef VULKAN
	uv.y*=-1;
#endif
	if(viewportSize.x>viewportSize.y)
		uv.x*=viewportSize.x/viewportSize.y;
	else
		uv.y*=viewportSize.y/viewportSize.x;
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
	vec2 mo = vec2(0.5);
	if(mouseButtonAction[0]==1)
		mo = mousePos;
	//vec2 mo=vec2(0);
	float theta=PI*(mo.y-0.5);
	float phi=2.*PI*mo.x-PI/2.;
	//eye parameters
	float ans=0;//.*iTime;

	ro=0.1*(mouseWheel[1]+30)*vec3(cos(phi)*cos(theta),sin(theta),sin(phi)*cos(theta));
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
	if(mouseButtonAction[0]==1)
		mo = mousePos;
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

vec3 getBackgroundColor(vec3 rd)
{
	return vec3(vec2(atan(rd.z,-1)/(2*PI)+0.5,acos(rd.y)/PI),0);
	//return textureLod(textures[1],vec2(atan(rd.z,rd.x)/(2*PI)+0.5,acos(rd.y)/PI),0).rgb;
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

	//step 1
	vec3 ro,rd;
	setCamera(uv,ro,rd);
	//setCameraOrtho(uv*CAMERA_DIST,ro,rd);
	
   	//vec3 col=vec3(ro.xyz);
   	vec3 col=getBackgroundColor(rd);

	fragColor=vec4(col,1);
}