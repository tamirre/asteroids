#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 textureSize; // set from Raylib: (texture.width, texture.height)

out vec4 finalColor;

void main()
{
    // Compute subpixel-correct UV sampling
    // Offsets half a pixel back into the texel to avoid sampling drift
    vec2 uv = fragTexCoord;
    uv = uv * textureSize - 0.5;
    uv = floor(uv) + fract(uv);
    uv = (uv + 0.5) / textureSize;

    finalColor = texture(texture0, uv) * fragColor;
}

