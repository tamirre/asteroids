#version 100
precision mediump float;
precision mediump int;

#extension GL_OES_standard_derivatives : enable

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 textureSize;
uniform float outlineSize;
uniform vec4 outlineColor;

vec2 aa(vec2 uv, vec2 size) {
    vec2 pixels = uv * size;
	pixels = floor(pixels) + min(fract(pixels) / fwidth(pixels), 1.0) - 0.5;
	// pixels = floor(pixels) + 0.5;
	return pixels / size;
}
void main()
{
    vec2 uv = aa(fragTexCoord, vec2(textureSize));
    vec4 texel = texture2D(texture0, uv); 
	vec2 texelScale = vec2(0.0);
    texelScale.x = outlineSize/float(textureSize.x);
    texelScale.y = outlineSize/float(textureSize.y);

	vec4 corners = vec4(0.0);
	corners.x = texture2D(texture0, fragTexCoord + vec2(texelScale.x, texelScale.y)).a;
	corners.y = texture2D(texture0, fragTexCoord + vec2(texelScale.x, -texelScale.y)).a;
	corners.z = texture2D(texture0, fragTexCoord + vec2(-texelScale.x, texelScale.y)).a;
	corners.w = texture2D(texture0, fragTexCoord + vec2(-texelScale.x, -texelScale.y)).a;

	float edges = 0.0;
	edges += texture2D(texture0, fragTexCoord + vec2( texelScale.x, 0.0)).a;
	edges += texture2D(texture0, fragTexCoord + vec2(-texelScale.x, 0.0)).a;
	edges += texture2D(texture0, fragTexCoord + vec2(0.0,  texelScale.y)).a;
	edges += texture2D(texture0, fragTexCoord + vec2(0.0, -texelScale.y)).a;

	// Combine both
	float outline = min(dot(corners, vec4(1.0)) + edges, 1.0);

    vec4 color = mix(vec4(0.0), outlineColor, outline);
    gl_FragColor = mix(color, texel, texel.a);
	
}
