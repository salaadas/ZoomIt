#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

uniform vec2  lampPos; // position
uniform vec2  imgSize;
uniform float radius;  // radius
uniform float scale;   // scale
uniform float shadow;  // scale
// texture sampler
uniform sampler2D img;

void main()
{
    vec4 lamp = vec4(lampPos.x, imgSize.y - lampPos.y, 0.0, 1.0);
    FragColor = mix(texture(img, TexCoord), vec4(0.0, 0.0, 0.0, 0.0),
                    length(lamp - gl_FragCoord) < (radius * scale) ? 0.0 : shadow);
	// FragColor = texture(img, TexCoord);
}
