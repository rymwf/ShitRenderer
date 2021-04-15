#version 460

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out VS_OUT { 
	vec3 color;
	vec2 texCoord;
}
vs_out;

void main() 
{
  gl_Position = vec4(inPos, 1);
  vs_out.color = inColor;
  vs_out.texCoord= inTexCoord;
}

