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
	//vec3 outColor = vec3(0);
	vec2 frag = vec2(gl_FragCoord);

	if (distance(frag, point) < radius) {
		outColor += density;
		//outColor = vec3(1);
	}

	//outColor = density;

	FragColor = vec4(outColor, 1.0);
}