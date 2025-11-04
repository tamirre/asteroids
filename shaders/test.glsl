#version 330

uniform sampler2D texture0;
uniform ivec2 textureSize; // <-- you must set this from C++
in vec2 fragTexCoord;
out vec4 finalColor;

vec2 uv_klems(vec2 uv, ivec2 textureSize) {
    vec2 pixels = uv * textureSize + 0.5;

    vec2 fl = floor(pixels);
    vec2 fr = fract(pixels);
    vec2 aa = fwidth(pixels) * 0.75;

    fr = smoothstep(vec2(0.5) - aa, vec2(0.5) + aa, fr);

    return (fl + fr - 0.5) / textureSize;
}

void main()
{
    vec2 uv = uv_klems(fragTexCoord, textureSize);
    finalColor = texture(texture0, uv);
}

