#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 blurTexCoords[11];
//out vec2 TexCoords;
flat out int Stage;

uniform float textureWidth;
uniform float textureHeight;

//stage refers to processing the x or y values
//0 is x 1 is y
uniform int stage;

void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);

    vec2 center = aPos * 0.5 + 0.5;

    float pixWidth = 1.0 / textureWidth;
    float pixHeight = 1.0 / textureHeight;

    if (stage == 0){
        //x values
        for (int i = -5; i <= 5; i++){
        	//set all the blur values based on the center pos
        	blurTexCoords[i + 5] = center + vec2(pixWidth * i, 0.0);
        }
    }
    else if (stage == 1){
        //y values
        for (int i = -5; i <= 5; i++){
        	//set all the blur values based on the center pos
        	blurTexCoords[i + 5] = center + vec2(0.0, pixHeight * i);
        }
    }

    //stage
    Stage = stage;

    //TexCoords = center;
    //TexCoords = aTexCoord;
}  