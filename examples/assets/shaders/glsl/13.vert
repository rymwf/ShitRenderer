#version 460

#ifdef VULKAN
#define SET(x) ,set=x
#else
#define SET(x)
#endif

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord0;
layout(location = 4) in vec2 inTexCoord1;
layout(location = 5) in vec4 inColor0;
#ifdef VULKAN
layout(location = 6) in uvec4 inJoints0;
#else
layout(location = 6) in vec4 inJoints0;
#endif
layout(location = 7) in vec4 inWeights0;
layout(location = 11) in vec4 inInstanceColorFactor;
layout(location = 12) in mat4 inInstanceMatrix;

layout(location = 0) out VS_OUT { 
	vec3 pos;
	vec3 normal;
	vec2 texCoord;
}
vs_out;

layout(binding=12 SET(0)) uniform UBOFrame{
	mat4 PV;
	vec3 eyePos;
};
layout(binding=13 SET(1)) uniform UBONode{
	mat4 M;
};

layout(constant_id=1) const int jointNum=32;	
layout(binding=15 SET(3)) uniform UBOJointMatrix
{
	mat4 jointMatrices[jointNum+1];
};
void main()
{
	mat4 tempMat=inInstanceMatrix*M;
	if(jointNum>0)
	{
		mat4 skinMatrix=
			inWeights0[0]*jointMatrices[int(inJoints0[0])]+ 
			inWeights0[1]*jointMatrices[int(inJoints0[1])]+ 
			inWeights0[2]*jointMatrices[int(inJoints0[2])]+ 
			inWeights0[3]*jointMatrices[int(inJoints0[3])];
		tempMat=tempMat*skinMatrix;
	}

	vec4 vertexPos=tempMat*vec4(inPos, 1);
	vs_out.pos= vertexPos.xyz;
	vs_out.normal= normalize(mat3(tempMat)*inNormal);
	vs_out.texCoord= inTexCoord0;

	gl_Position = PV*vertexPos;
}