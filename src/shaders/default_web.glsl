#version 100
precision mediump float;
precision mediump int;

#extension GL_OES_standard_derivatives : enable

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec2 textureSize;

vec2 uv_klems(vec2 uv, ivec2 textureSize)
{
    vec2 pixels = uv * vec2(textureSize) + 0.5;

    vec2 fl = floor(pixels);
    vec2 fr = fract(pixels);
    vec2 aa = fwidth(pixels) * 0.75;

    fr = smoothstep(vec2(0.5) - aa, vec2(0.5) + aa, fr);

    return (fl + fr - 0.5) / vec2(textureSize);
}
vec2 aa(vec2 uv, vec2 size) {
    vec2 pixels = uv * size;
	pixels = floor(pixels) + min(fract(pixels) / fwidth(pixels), 1.0) - 0.5;
	// pixels = floor(pixels) + 0.5;
	return pixels / size;
}
void main()
{
    vec2 uv = aa(fragTexCoord, textureSize);
	gl_FragColor = texture2D(texture0, uv) * fragColor;
}

