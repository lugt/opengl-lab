#version 330 core

in vec3 vPosition;
in vec3 vColor;
out vec3 color;

uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(vPosition, 1.0);
//    TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    color = vColor;
}
