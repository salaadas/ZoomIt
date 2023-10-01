#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

uniform vec2  camera;
uniform float scale;
uniform vec2  imgSize;

vec3 convert(vec3 v)
{
    return vec3((v.x + camera.x / imgSize.x),
                (v.y - camera.y / imgSize.y),
                v.z) * vec3(scale, scale, 1.0);
}

void main()
{
	gl_Position = vec4(convert(aPos), 1.0);
	ourColor = aColor;
	TexCoord = aTexCoord;
}
