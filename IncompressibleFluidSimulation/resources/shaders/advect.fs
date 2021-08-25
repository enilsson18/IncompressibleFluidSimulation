#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// timestep
uniform float dt;
// single texel size
uniform float rdx;

uniform sampler2D v;
// applies the changes to d
uniform sampler2D d;

float floorTo(float num, float inc){
	return num - mod(num, inc);
}

vec4 advect(sampler2D v, sampler2D d, vec2 coords, float dt, float rdx) {
	vec2 pos = coords - dt * rdx * 0 * vec2(texture(v, coords));

	// weight each neighboring texel based on how close it is to the downstream position
	float i0 = floorTo(pos.x, rdx);
	float i1 = i0 + rdx;
	float j0 = floorTo(pos.y, rdx);
	float j1 = j0 + rdx;

	float s1 = pos.x - i0;
	float s0 = rdx - s1;
	float t1 = pos.y - j0;
	float t0 = rdx - t1;

	vec2 i0j0 = vec2(i0, j0);
	vec2 i0j1 = vec2(i0, j1);
	vec2 i1j0 = vec2(i1, j0);
	vec2 i1j1 = vec2(i1, j1);

	return texture(d, TexCoords);

	return 
		s0 * (t0 * texture(d, i0j0) + t1 * texture(d, i0j1)) +
		s1 * (t0 * texture(d, i1j0) + t1 * texture(d, i1j1));
}

void main(){
	FragColor = vec4(vec3(advect(v, d, TexCoords, dt, rdx)), 1.0);
	FragColor = vec4(vec3(texture(d, TexCoords)), 1.0);
}
