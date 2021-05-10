#version 460
layout(location = 0) out vec4 outColor;
layout(location = 0) in VS_OUT { 
	vec3 color; 
	vec2 texCoord;
}fs_in;

layout(binding=0) uniform sampler2D tex;
//layout(binding=0) uniform sampler2DArray tex;

void main() 
{
//   outColor = vec4(fs_in.color, 1);
//  outColor = texture(tex,fs_in.texCoord);
if(gl_FragCoord.x>640)
{
  outColor = textureLod(tex,fs_in.texCoord,5);
  //outColor = textureLod(tex,vec3(fs_in.texCoord,0),5);
}
else
{
  //outColor = textureLod(tex,vec3(fs_in.texCoord,1),0);
  outColor = textureLod(tex,fs_in.texCoord,0);
}
}
