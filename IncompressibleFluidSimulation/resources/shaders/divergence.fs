#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// single texel size
uniform float rdx;

// tex A is the texture that will be changed and tex B is the previo
uniform sampler2D tex;

vec4 divergence(sampler2D tex, vec2 coords, float rdx){
	// subract to account for veleocity offset
	return (
		0.5 * (
			  (texture(tex, coords + vec2( 0,  1) * rdx) - vec4(vec2(0.5), 0, 0))
			- (texture(tex, coords + vec2( 0, -1) * rdx) - vec4(vec2(0.5), 0, 0))
			+ (texture(tex, coords + vec2( 1,  0) * rdx) - vec4(vec2(0.5), 0, 0))
			- (texture(tex, coords + vec2(-1,  0) * rdx) - vec4(vec2(0.5), 0, 0))
		) + 0.5
	);
}

void main(){
	FragColor = vec4(vec3(divergence(tex, TexCoords, rdx)), 1.0);
}