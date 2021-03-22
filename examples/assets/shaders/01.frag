#version 450
layout(location = 0) out vec4 outColor;

struct VS_OUT{
	vec4 color;
};

layout(location=0)in VS_OUT fs_in;
void main() { outColor = fs_in.color;}
