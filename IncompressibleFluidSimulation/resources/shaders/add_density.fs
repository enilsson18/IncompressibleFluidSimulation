#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// offset for each side
uniform vec2 point;

uniform vec3 density;

uniform float radius;

// the current tex of the map
uniform sampler2D tex;

void main(){
	vec3 outColor = vec3(texture(tex, TexCoords));

	if (distance(vec2(gl_FragCoord), point) < 0.1) {
		outColor += density;
	}

	//outColor = density;

	FragColor = vec4(outColor, 1.0f);
}