#version 330

uniform sampler2D texture0;
uniform vec2 textureSize;

in vec2 fragTexCoord;
out vec4 finalColor;

vec2 stable_uv(vec2 uv)
{
    // convert to texel space
    vec2 texel = uv * textureSize;

    // texel center
    vec2 base = floor(texel) + 0.5;

    // derivative of texel coordinates
    vec2 d = fwidth(texel);

    // offset from center
    vec2 f = texel - base;

    // clamp offset to reduce drift between pixels
    f = clamp(f / d, -0.5, 0.5) * d;

    texel = base + f;

    return texel / textureSize;
}

void main()
{
    vec2 uv = stable_uv(fragTexCoord);
    finalColor = texture(texture0, uv);
}
