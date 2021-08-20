#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform float dt;
uniform float diff;
uniform float size;

// tex A is the texture that will be changed and tex B is the previo
uniform sampler2D texA;
uniform sampler2D texB;

vec4 diffuse(sampler2D texA, sampler2D texB, vec2 coords, float dt, float diff, float size){
	float a = dt * diff * (size - 2) * (size - 2);
	float recip = 1 / (1 + 4 * a);

	return (
		recip * texture(texB, coords) + 
		a * (
			texture(texA, coords + vec2(0, 1) +
			texture(texA, coords + vec2(0, -1) +
			texture(texA, coords + vec2(1, 0) +
			texture(texA, coords + vec2(-1, 0)
		)
	);
}

void main(){
	FragColor = vec4(vec3(diffuse(texA, texB, TexCoords, dt, diff, size)), 1.0);
}