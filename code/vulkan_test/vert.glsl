#version 450

layout(location = 0) in vec3 in_position; 
layout(location = 1) in vec3 in_color; 

layout(location = 0) out vec3 frag_color;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 clip;
} ubo;

void main() 
{
    gl_Position = ubo.clip * ubo.proj * ubo.view * ubo.model * vec4(in_position, 1.0);
    frag_color = in_color;
}
