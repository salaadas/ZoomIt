#version 330 core
out vec4 FragColor;

uniform float iTime;

in vec3 ourColor;
in vec2 TexCoord;

void main()
{
    // FragColor =vec4((sin(TexCoord.x + iTime) + 1.0) / 2.0,
    //                 (-sin(TexCoord.y + iTime) + 1.0) / 2.0,
    //                 (-cos(TexCoord.x + TexCoord.y + iTime) + 1.0) / 2.0,
    //                 1.0);
    FragColor = vec4(ourColor, 1.0);
}
