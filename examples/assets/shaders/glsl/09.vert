#version 450
//#extension all: warn

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
	vec4 colorFactor;
	vec2 texCoord;
	vec3 pos;
	vec3 normal;
}
vs_out;

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
	vec3 eyePosition;
	Light light;
	vec3 ambientColor;
};

layout(binding=13 SET(1)) uniform UBONode{
	mat4 M;
};

//NOTE: in opengl ,uniform block have a static layout, changing specialized size will not re-layout the block
// the block will based on the default size of the array
//TODO: may not work in opengl
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
	gl_Position = PV*vertexPos;

	//const float a=float(jointNum)/5.;
	vs_out.colorFactor = inInstanceColorFactor;
	//vs_out.colorFactor = vec4(vec3(a),1);//inInstanceColorFactor;//*mix(vec4(1),inColor0,step(0.001,inColor0.r+inColor0.g+inColor0.b));
	//vs_out.colorFactor = vec4(vec3(float(jointNum)/5),1);//inInstanceColorFactor;//*mix(vec4(1),inColor0,step(0.001,inColor0.r+inColor0.g+inColor0.b));
	vs_out.texCoord= inTexCoord0;
	vs_out.pos= vertexPos.xyz;
	vs_out.normal= normalize(mat3(tempMat)*inNormal);
}
