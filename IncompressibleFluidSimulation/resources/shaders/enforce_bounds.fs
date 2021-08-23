#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// for velocity, it is set to -1 and for pressure, it is set to 1
uniform float scale;

// offset for each side
uniform vec2 offset;

// the input velocity or pressure map
uniform sampler2D tex;

void main(){
	FragColor = scale * texture(tex, TexCoords * offset);
}