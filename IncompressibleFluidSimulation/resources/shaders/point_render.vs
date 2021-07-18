#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out vec3 Color;

void main()
{
    gl_Position = vec4((aPos.x*2) - 1, (aPos.y*2)-1, 0.0, 1.0); 
    Color = aColor;
}