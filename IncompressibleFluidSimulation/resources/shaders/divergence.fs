#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// single texel size
uniform float rdx;

// tex A is the texture that will be changed and tex B is the previo
uniform sampler2D tex;

vec4 divergence(sampler2D tex, vec2 coords, float rdx){
	return (
		0.5 * rdx * (
			  texture(tex, coords + vec2(0, 1))
			- texture(tex, coords + vec2(0, -1))
			+ texture(tex, coords + vec2(1, 0))
			- texture(tex, coords + vec2(-1, 0))
		)
	);
}

void main(){
	FragColor = vec4(vec3(divergence(tex, TexCoords, rdx)), 1.0);
}