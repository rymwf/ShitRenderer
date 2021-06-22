#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif

layout(location = 0) in vec3 inPos;
#ifdef VULKAN
layout(location = 5) in uvec4 inJoints0;
#else
layout(location = 5) in vec4 inJoints0;
#endif
layout(location = 6) in vec4 inWeights0;
layout(location = 12) in mat4 inInstanceMatrix;

struct Light{
	vec3 pos;
	vec4 color;
	vec4 intensity;
	vec2 lim_r; //the attenuation distance limit,r.x: min dist(sphere light radius), r.y: max dist
	vec3 direction;
	vec3 tube_p0;
	vec3 tube_p1;
};
layout(binding=12 SET(0)) uniform UBOFrame{
	mat4 PV;
	Light light;
	vec3 ambientColor;
};
layout(binding=13 SET(1)) uniform UBONode{
	mat4 M;
};
layout(constant_id=1) const int jointNum=30;
layout(binding=15 SET(3)) uniform UBOJointMatrix
{
	mat4 jointMatrices[jointNum+1];
};

void main() 
{
	mat4 tempMat=inInstanceMatrix*M;
	if(bool(jointNum))
	{
		mat4 skinMatrix=
			inWeights0[0]*jointMatrices[int(inJoints0[0])]+ 
			inWeights0[1]*jointMatrices[int(inJoints0[1])]+ 
			inWeights0[2]*jointMatrices[int(inJoints0[2])]+ 
			inWeights0[3]*jointMatrices[int(inJoints0[3])];
		tempMat=tempMat*skinMatrix;
	}

	vec4 vertexPos=tempMat*vec4(inPos, 1);
	gl_Position = PV*vertexPos;
	//vs_out.colorFactor = inInstanceColorFactor;
	//vs_out.texCoord= inTexCoord1;
	//vs_out.pos= vertexPos.xyz;
	//vs_out.normal= mat3(tempMat)*inNormal;
}
