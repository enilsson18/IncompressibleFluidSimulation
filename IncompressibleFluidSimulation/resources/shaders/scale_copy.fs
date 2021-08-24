#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// the ratio new size / old size
uniform float scale;

uniform sampler2D tex;

void main(){
	vec4 color = texture(tex, TexCoords * scale);
	FragColor = vec4(vec3(color), 1.0);
}