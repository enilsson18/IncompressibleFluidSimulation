#version 330 core

out vec3 FragColor;

in vec2 blurTexCoords[11];
//in vec2 TexCoords;
flat in int Stage;

uniform sampler2D tex;

void main()
{   
	//output the color and sample from all the surrounding textures
	FragColor = vec3(0.0);

	//11x11 blur 2 pass style
	//weights for averaging so that the end result is a blur and not a block look on wiki im too tired to explain it is called kernel distribution in gaussian blurring or something
	//http://dev.theomader.com/gaussian-kernel-calculator/
	float[11] kernel = float[11](0.000003, 0.000229, 0.005977, 0.060598, 0.24173, 0.382925, 0.24173, 0.060598, 0.005977, 0.000229, 0.000003);

	//add the value scaled down based on the number of extra samples we are drawing from
	//if you were wondering i access the r value since it is the first value of the vec4 and I am inputing a single value float map into is so only r in rbg is nonzero
	for (int i = 0; i < 11; i++){
		FragColor += (texture(tex, blurTexCoords[i]) * kernel[i]).r;
	}

	//transform to rgb space
	if (Stage == 1){
		FragColor *= 255;
	}

	//debug test values
	//FragColor = vec4(0.0);
	//FragColor = vec3(texture(tex, TexCoords).r);
}  