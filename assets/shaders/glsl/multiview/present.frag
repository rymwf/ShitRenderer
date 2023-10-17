#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif
//layout(early_fragment_tests)in;

layout(location=0) out vec4 outColor;

layout(location =0) in vec2 fs_texcoord;
layout(binding=0) uniform sampler2DArray tex;

//uv  is [0,1]
vec2 distortUV(vec2 uv,float distortion)
{
	vec2 p=uv*2-1;
	vec2 p2=p/(1-distortion*length(p));
	p2=(p2+1)*0.5;
	return p2;
}

void main()
{
	outColor=vec4(0);
	///outColor=vec4(texture(tex,vec3(fs_in.texcoord,0)).rgb,1);
	float distortion=0.3;
	if(fs_texcoord.x>0.5)
	{
		vec2 p=distortUV(vec2(2*fs_texcoord.x-1,fs_texcoord.y),distortion);
		if(p.x<=1&&p.x>=0&&p.y>=0&&p.y<=1)
			outColor = texture(tex, vec3(p, 1));
	}else
	{
		vec2 p=distortUV(vec2(2*fs_texcoord.x,fs_texcoord.y),distortion);
		if(p.x<=1&&p.x>=0&&p.y>=0&&p.y<=1)
			outColor = texture(tex, vec3(p, 0));
	}
}