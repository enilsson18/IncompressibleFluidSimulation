#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// single texel size
uniform float rdx;

// float a = dt * diff * (size - 2) * (size - 2);
uniform float a;

// float recip = 1 / (1 + 4 * a);
uniform float recip;

// tex A is the texture that will be changed and tex B is the previo
uniform sampler2D tex;

vec4 jacobi(sampler2D tex, vec2 coords, float rdx, float a, float recip){
	return recip * (
		texture(tex, coords) + 
		a * (
			texture(tex, coords + rdx * vec2(0, 1)) +
			texture(tex, coords + rdx * vec2(0, -1)) +
			texture(tex, coords + rdx * vec2(1, 0)) +
			texture(tex, coords + rdx * vec2(-1, 0))
		)
	);
}

void main(){
	FragColor = vec4(vec3(jacobi(tex, TexCoords, rdx, a, recip)), 1.0);
	//FragColor = vec4(vec3(texture(texA, TexCoords + vec2(0.01))), 1.0);
}