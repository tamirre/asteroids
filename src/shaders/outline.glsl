#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 textureSize;
uniform float outlineSize;
uniform vec4 outlineColor;

// Output fragment color
out vec4 finalColor;

vec2 aa(vec2 uv, vec2 size) {
    vec2 pixels = uv * size;
	pixels = floor(pixels) + min(fract(pixels) / fwidth(pixels), 1.0) - 0.5;
	// pixels = floor(pixels) + 0.5;
	return pixels / size;
}

void main()
{
    vec2 uv = aa(fragTexCoord, vec2(textureSize));
    vec4 texel = texture(texture0, uv);
    vec2 texelScale = vec2(0.0);
    texelScale.x = outlineSize/textureSize.x;
    texelScale.y = outlineSize/textureSize.y;

	vec4 corners = vec4(0.0);
	corners.x = texture(texture0, fragTexCoord + vec2(texelScale.x, texelScale.y)).a;
	corners.y = texture(texture0, fragTexCoord + vec2(texelScale.x, -texelScale.y)).a;
	corners.z = texture(texture0, fragTexCoord + vec2(-texelScale.x, texelScale.y)).a;
	corners.w = texture(texture0, fragTexCoord + vec2(-texelScale.x, -texelScale.y)).a;

	float edges = 0.0;
	edges += texture(texture0, fragTexCoord + vec2( texelScale.x, 0.0)).a;
	edges += texture(texture0, fragTexCoord + vec2(-texelScale.x, 0.0)).a;
	edges += texture(texture0, fragTexCoord + vec2(0.0,  texelScale.y)).a;
	edges += texture(texture0, fragTexCoord + vec2(0.0, -texelScale.y)).a;

	// Combine both
	float outline = min(dot(corners, vec4(1.0)) + edges, 1.0);

    vec4 color = mix(vec4(0.0), outlineColor, outline);
    finalColor = mix(color, texel, texel.a);
}
