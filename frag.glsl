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

// const int indexMatrix4x4[16] = int[](0,  8,  2,  10,
//                                      12, 4,  14, 6,
//                                      3,  11, 1,  9,
//                                      15, 7,  13, 5);

// float indexValue()
// {
//     int x = int(mod(gl_FragCoord.x, 4));
//     int y = int(mod(gl_FragCoord.y, 4));
//     return indexMatrix4x4[(x + y * 4)] / 16.0;
// }

// float dither(float color)
// {
//     float closestColor = (color < 0.5) ? 0 : 1;
//     float secondClosestColor = 1 - closestColor;
//     float d = indexValue();
//     float distance = abs(closestColor - color);
//     return ((distance < d) ? closestColor : secondClosestColor);
// }

void main()
{
    // vec4 color = texture2D(img, TexCoord);
    // vec3 c = vec3(dither(color.x), dither(color.y), dither(color.z));
    // FragColor = (c, 1.0);

    vec4 lamp = vec4(lampPos.x, imgSize.y - lampPos.y, 0.0, 1.0);
    FragColor = mix(texture(img, TexCoord), vec4(0.0, 0.0, 0.0, 0.0),
                    length(lamp - gl_FragCoord) < (radius * scale) ? 0.0 : shadow);
	// FragColor = texture(img, TexCoord);
}
