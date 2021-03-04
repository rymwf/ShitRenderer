#version 450
layout(location = 0) out vec4 outColor;
layout(location = 0) in VS_OUT { vec3 color; }
fs_in;
void main() {
   outColor = vec4(fs_in.color, 1);
//  outColor = vec4(1, 0, 0, 1);
}
