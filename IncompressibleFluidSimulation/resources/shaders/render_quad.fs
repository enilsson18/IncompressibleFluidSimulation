#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex;

void main()
{             
    FragColor = vec4(vec3(texture(tex, TexCoords)), 1.0);
    //FragColor = vec4(vec3(1.0, 0.0, 0.0), 1.0);
}