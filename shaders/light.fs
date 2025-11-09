#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 lightPos;      // normalized 0-1
uniform float lightRadius;  // normalized
uniform float aspect;       // screenWidth / screenHeight

void main()
{
    vec4 base = texture(texture0, fragTexCoord);

    // Fix distortion: scale X by aspect ratio
    vec2 p = fragTexCoord;
    p.x *= aspect;
    vec2 lp = lightPos;
    lp.x *= aspect;

    float dist = distance(p, lp);
    float intensity = 1.0 - smoothstep(0.0, lightRadius, dist);

    vec3 lit = base.rgb * (0.3 + intensity * 1.7);
    finalColor = vec4(lit, base.a);
}

