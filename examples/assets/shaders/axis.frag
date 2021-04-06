#version 460
layout(location=0)out vec4 outColor;

layout(location=0)in VS_OUT{
	vec3 pos;
}fs_in;
void main() 
{ 
	outColor=vec4(fs_in.pos,1);
}
