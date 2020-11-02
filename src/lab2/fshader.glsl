#version 330 core

in vec3 color;
out vec4 fColor;


// texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    fColor = vec4(color, 1.0);
}
