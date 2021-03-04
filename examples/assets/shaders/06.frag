#version 330
#extension all:warn

layout(location = 0) out vec4 outColor;

layout(location = 0) in VS_OUT { 
	vec3 color;
	vec2 texCoord;
}fs_in;

layout(binding=1) uniform sampler2D tex;

void main() {
   //outColor = vec4(fs_in.color, 1);
   //outColor = vec4(fs_in.texCoord,0,h1);
   outColor = texture(tex,fs_in.texCoord);
}
