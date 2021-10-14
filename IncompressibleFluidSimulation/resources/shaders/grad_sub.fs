#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// single texel size
uniform float rdx;

// the input velocity map
uniform sampler2D texA;

// the input pressure map
uniform sampler2D texB;

vec4 gradSub(sampler2D texA, sampler2D texB, vec2 coords, float rdx){
	float t = (texture(texB, coords + vec2(0, 1) * rdx).x - 0.5);
	float b = (texture(texB, coords + vec2(0, -1) * rdx).x - 0.5);
	float r = (texture(texB, coords + vec2(1, 0) * rdx).x - 0.5);
	float l = (texture(texB, coords + vec2(-1, 0) * rdx).x - 0.5);

	vec4 currentVelocity = texture(texA, coords);

	return currentVelocity - 0.1 * 0.5 * vec4(r - l, t - b, 0, 0);
}

void main(){
	FragColor = vec4(vec3(gradSub(texA, texB, TexCoords, rdx)), 1.0);
}