#version 460

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif

layout(triangles) in;
layout(triangle_strip,max_vertices=30) out;

layout(SET(1) binding=0) buffer UBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
	int cascadeNum;
	float cascadeSplit[];
}uboCamera;

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

	vec4 vertices[4];
	mat4 PV[];
}uboLight;

layout(location =0) in vec2 vs_texcoords[];

layout (location=0) out vec2 gs_texcoord;

void main()
{
	int count=1;
	if(uboLight.lightType==1)
		count=uboCamera.cascadeNum;
	else if(uboLight.lightType==2)
		count=6;

	for(int i=0;i<count;++i)
	{
		gl_Layer=i;
		for(int j=0;j<3;++j)
		{
			gl_Position=uboLight.PV[2*i]*uboLight.PV[i*2+1]*gl_in[j].gl_Position;
			gs_texcoord=vs_texcoords[j];
			EmitVertex();
		}
		EndPrimitive();
	}
}