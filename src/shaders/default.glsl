#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 textureSize; 

vec2 uv_klems(vec2 uv, ivec2 textureSize) {
    vec2 pixels = uv * textureSize + 0.5;

    vec2 fl = floor(pixels);
    vec2 fr = fract(pixels);
    vec2 aa = fwidth(pixels) * 0.75;

    fr = smoothstep(vec2(0.5) - aa, vec2(0.5) + aa, fr);

    return (fl + fr - 0.5) / textureSize;
}
vec2 aa(vec2 uv, vec2 size) {
    vec2 pixels = uv * size;
	pixels = floor(pixels) + min(fract(pixels) / fwidth(pixels), 1.0) - 0.5;
	// pixels = floor(pixels) + 0.5;
	return pixels / size;
}
void main()
{
    vec2 uv = aa(fragTexCoord, vec2(textureSize));
    // vec2 uv = uv_klems(fragTexCoord, textureSize);

    vec4 tmp = texture(texture0, uv);
    // vec4 tmp = texture(texture0, fragTexCoord);
	// discard;
	// tmp.r = 1.0;
	finalColor = tmp;
	// finalColor = vec4(1.0, 0.0, 0.0, 1.0);

}

