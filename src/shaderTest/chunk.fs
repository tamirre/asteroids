#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;
uniform float explode;

// --- better hash ---
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main()
{
    float gridSize = 12.0;

    vec2 gridUV = fragTexCoord * gridSize;
    vec2 cell = floor(gridUV);
    vec2 localUV = fract(gridUV);

    // --- RANDOM DIRECTION (not axis-aligned) ---
    float angle = hash(cell) * 6.2831853;

    // avoid near-axis directions by biasing angle slightly
    angle += 0.3;

    vec2 dir = normalize(vec2(cos(angle), sin(angle)));

    // --- movement grows over time ---
	float speed = mix(0.5, 2.0, hash(cell + 2.0));
	vec2 offset = dir * explode * speed;
    vec2 finalUV = (cell + localUV + offset) / gridSize;

    // sample texture
    vec4 col = texture(texture0, finalUV) * fragColor;

    // --- alpha fade based on explosion distance ---
    float fade = 1.0 - smoothstep(0.5, 3.0, explode);

    col.a *= fade;

    // discard fully transparent pixels early
    if (col.a < 0.01)
        discard;

    finalColor = col;
}
