#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// single texel size
uniform float rdx;

// float a = dt * diff * (size - 2) * (size - 2);
uniform float a;

// float recip = 1 / (1 + 4 * a);
uniform float recip;

uniform int texCount = 1;

// tex A is the texture that will be changed and tex B is the previous
uniform sampler2D texA;
uniform sampler2D texB;

vec4 jacobi(sampler2D texA, sampler2D texB, vec2 coords, float rdx, float a, float recip){
	vec4 toAdd;

	if (texCount > 1){
		toAdd = texture(texB, coords);
	}
	else {
		toAdd = texture(texA, coords);
	}

	return recip * (
		toAdd + 
		a * (
			texture(texA, coords + rdx * vec2(0, 1)) +
			texture(texA, coords + rdx * vec2(0, -1)) +
			texture(texA, coords + rdx * vec2(1, 0)) +
			texture(texA, coords + rdx * vec2(-1, 0))
		)
	);
}

void main(){
	FragColor = vec4(vec3(jacobi(texA, texB, TexCoords, rdx, a, recip)), 1.0);
	//FragColor = vec4(vec3(texture(texA, TexCoords + vec2(0.01))), 1.0);
}