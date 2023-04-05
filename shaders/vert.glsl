#version 450

layout(binding = 0) uniform UniformBufferObject {
  mat4 model;
  mat4 view;
  mat4 projection;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_colour;

layout(location = 0) out vec3 frag_colour;

void main() {
  gl_Position = ubo.projection * ubo.view * ubo.model * vec4(in_position, 1.0);
  frag_colour = in_colour;
}