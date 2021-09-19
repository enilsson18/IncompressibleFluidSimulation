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

vec2 floorVec2To(vec2 vector, float inc){
	return vec2(floorTo(vector.x, inc), floorTo(vector.y, inc));
}

vec4 advect(sampler2D v, sampler2D d, vec2 coords, float dt, float rdx) {
	//vec2 coords2 = gl_FragCoord.xy * rdx;

	float rRDX = 1 / rdx;

	vec2 pos = (coords * rRDX - dt * 10 * (texture(v, coords).xy - vec2(0.5)));

	//return vec4(coords, 0, 0);
	//return vec4((texture(v, coords).xy - vec2(0.5)), 0.0, 1.0);

	// weight each neighboring texel based on how close it is to the downstream position
	float i0 = floor(pos.x);
	float i1 = i0 + 1.0;
	float j0 = floor(pos.y);
	float j1 = j0 + 1.0;

	float s1 = pos.x - i0;
	float s0 = 1.0 - s1;
	float t1 = pos.y - j0;
	float t0 = 1.0 - t1;

	vec2 i0j0 = floorVec2To(vec2(i0, j0) * rdx, rdx);
	vec2 i0j1 = floorVec2To(vec2(i0, j1) * rdx, rdx);
	vec2 i1j0 = floorVec2To(vec2(i1, j0) * rdx, rdx);
	vec2 i1j1 = floorVec2To(vec2(i1, j1) * rdx, rdx);

	//return vec4(-(pos - coords), 0, 0);
	//return texture(d, pos);

	//return vec4(vec2(s0,s1) * 1000000, 0, 0);

	//return 
	//	s0 * (t0 * texture(d, pos) + t1 * texture(d, pos)) +
	//	s1 * (t0 * texture(d, pos) + t1 * texture(d, pos));
	//return vec4(i1j0, 0, 1);

	return 
		s0 * (t0 * texture(d, i0j0) + t1 * texture(d, i0j1)) +
		s1 * (t0 * texture(d, i1j0) + t1 * texture(d, i1j1));
}

void main(){
	FragColor = vec4(vec3(advect(v, d, TexCoords, dt, rdx)), 1.0);
	//vec4 color = texture(d, TexCoords);
    //FragColor = vec4(vec3(color), 1.0);
}
