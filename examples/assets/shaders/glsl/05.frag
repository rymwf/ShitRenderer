#version 460
layout(location = 0) out vec4 outColor;
layout(location = 0) in VS_OUT { 
	vec3 color; 
	vec2 texCoord;
}fs_in;

layout(binding=0) uniform sampler2D tex;

void main() 
{
//   outColor = vec4(fs_in.color, 1);
  outColor = texture(tex,fs_in.texCoord);
}
