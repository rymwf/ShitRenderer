#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out VS_OUT { vec3 color; }
vs_out;

// std140
layout(binding = 1) uniform UBO_MVP {
  mat4 M;
  mat4 V;
  mat4 P;
};

void main() {
  gl_Position = P * V * M * vec4(inPos, 1);
  //gl_Position =  vec4(inPos, 0, 1);
  vs_out.color = inColor;
}
