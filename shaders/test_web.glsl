#version 100
precision mediump float;
precision mediump int;

#extension GL_OES_standard_derivatives : enable

varying vec2 fragTexCoord;

uniform sampler2D texture0;
uniform ivec2 textureSize;

vec2 uv_klems(vec2 uv, ivec2 textureSize)
{
    vec2 pixels = uv * vec2(textureSize) + 0.5;

    vec2 fl = floor(pixels);
    vec2 fr = fract(pixels);
    vec2 aa = fwidth(pixels) * 0.75;

    fr = smoothstep(vec2(0.5) - aa, vec2(0.5) + aa, fr);

    return (fl + fr - 0.5) / vec2(textureSize);
}

void main()
{
    vec2 uv = uv_klems(fragTexCoord, textureSize);
    gl_FragColor = texture2D(texture0, uv);
}

