#version 460
layout(location=0) in vec2 inTexcoord;
layout(location=0) out vec4 fragColor;
layout(binding=0) uniform sampler2D tex;

#ifdef VULKAN
#define SET(x) set=x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#extension GL_EXT_scalar_block_layout: enable
#define PUSH_CONSTANT std430,binding=1
#endif
layout(PUSH_CONSTANT) uniform uPushConstant{
	int channel;
	int mode;
	float saturationFactor;
	float exposure;
	float factor0;
	float factor1;
	int intfactor0;
	int acesFilm;
	int quantityK;
};

#define PI 3.141592653
#define EPSILON 1e-6
#define DEG2RAD(x) x*0.01745329251994329576
#define RAD2DEG(x) x*57.2957795130823208767

void f_convertToCMY(inout vec3 col)
{
	col=1.-col;
}
void f_log(inout vec3 col)
{
	col=log(1+col);
}
void f_power(inout vec3 col)
{
	col=pow(col,vec3(factor0));
}
void f_constrast_smoothstep(inout vec3 col)
{
	col=smoothstep(vec3(0),vec3(1),col);
}
void f_intensity_slice(inout vec3 col)
{
	float a=col.r;
	int channel=intfactor0;
	if(channel==1)
		a=col.g;
	else if(channel==2)
		a=col.b;
	if(a>=factor0&&a<=factor1)
		col=vec3(0,1,0);
}
vec3 ACESFilm(vec3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x*(a*x+b))/(x*(c*x+d)+e),0,1);
}

//RGB
float grayScale(vec3 x)
{
	return dot(vec3(0.2126,0.7152,0.0722),x);
}
vec3 saturation(vec3 x,float factor)
{
	return mix(x,vec3(grayScale(x)),factor);
}

//0-1
float getHue(vec3 rgb)
{
	float R_G=rgb.r-rgb.g;
	float R_B=rgb.r-rgb.b;
	float G_B=rgb.g-rgb.b;

	float theta=acos((R_G+R_B)*0.5/(sqrt(R_G*R_G+R_B*G_B)+EPSILON));
	return mix(2.*PI-theta,theta,step(0,G_B))/(2.*PI);
}
float getSaturation(vec3 rgb)
{
	return 1.-3.*min(rgb.r,min(rgb.g,rgb.b))/(rgb.r+rgb.g+rgb.b);
}
float getIntensity(vec3 rgb)
{
	return (rgb.r+rgb.g+rgb.b)/3.;
}

void f_convertToHSI(inout vec3 col)
{
	col=vec3(getHue(col),getSaturation(col),getIntensity(col));
}

//h [0 1]
void HSI2RGB(inout vec3 hsi)
{
	vec3 col;
	float a=hsi.x*3;
	float h=fract(a)/3.;
	//if(abs(hsi.x-1)<1e-6)
	if(hsi.x==1.)
		h=1./3.;	
	col.b=hsi.z*(1-hsi.y);
	col.r=hsi.z*(1+hsi.y*cos(h*2*PI)/cos(PI/3.-h*2*PI));
	col.g=3*hsi.z-col.b-col.r;

	if(a<1)
		hsi=col;
	else if(a<2)
		hsi=col.brg;
	else
		hsi=col.gbr;
}

void f_quantization(inout vec3 col,int intensityLevel)
{
	f_convertToHSI(col);
	col.b=float(round(col.b*intensityLevel))/intensityLevel;
	HSI2RGB(col);
}

void main()
{
	vec4 col=texture(tex,inTexcoord);

	switch(mode)
	{
		case 1:
		f_convertToCMY(col.rgb);	
		break;
		case 2:
		f_convertToHSI(col.rgb);	
		break;
		case 3:
		f_log(col.rgb);
		break;
		case 4:
		f_power(col.rgb);
		break;
		case 5:
		f_constrast_smoothstep(col.rgb);
		break;
		case 6:
		f_intensity_slice(col.rgb);
		break;
		case 7:
		f_quantization(col.rgb,int(pow(2,quantityK)));
		break;
	}

	switch(channel)
	{
		case 1:
		col.rgb=vec3(col.r);
		break;
		case 2:
		col.rgb=vec3(col.g);
		break;
		case 3:
		col.rgb=vec3(col.b);
		break;
		case 4:
		col.rgb=vec3(col.a);
		break;
	}
	//col.rgb=grayScale(col.rgb);
	col.rgb=saturation(col.rgb,saturationFactor);
	col.rgb*=exposure;
	if(bool(acesFilm))
		col.rgb=ACESFilm(col.rgb);
	fragColor=vec4(col.rgb,1);
}