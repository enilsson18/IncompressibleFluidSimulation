#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

// for velocity, it is set to -1 and for pressure, it is set to 1
uniform float scale;

// offset for each side
uniform vec2 offset;

// the input velocity or pressure map
uniform sampler2D tex;

// tells whether or not to handle the 0.5 scaling since textures can't have negative numbers.
uniform int useHalf = 0;

void main(){
	vec4 color = texture(tex, TexCoords + offset);

	if (useHalf == 1){
		vec4 h = vec4(vec2(0.5), vec2(0));
		FragColor = (scale * (color - h)) + h;
	}
	else {
		FragColor = scale * color;
	}
}