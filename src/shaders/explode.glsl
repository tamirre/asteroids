#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float progress;

out vec4 finalColor;

// --- simple hash ---
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    vec2 UV = fragTexCoord;

    // texture size
    vec2 texSize = vec2(textureSize(texture0, 0));

    // --- chunk size (bigger = chunkier explosion) ---
    float chunkSize = 3.0;

    // snap to chunk grid
    vec2 grid = floor(UV * texSize / chunkSize);

    // stable UV per chunk
    vec2 chunkUV = grid * chunkSize / texSize;

    // random per chunk
    float r = rand(grid);

    // --- outward direction from center ---
    // vec2 center = vec2(0.5, 0.5);
    // vec2 dir = normalize(chunkUV - center);

    // add slight randomness
    // dir += (vec2(rand(grid + 1.3), rand(grid + 2.1)) - 0.5) * 0.8;
    // dir = normalize(dir);

    vec2 dir = vec2(0.0, 1.0);

    // --- hardcoded explosion strength ---
    float strength = 0.8;

    float move = progress/100.0 * strength * (0.5 + r);

    vec2 uv = chunkUV - dir * move;

    // sample texture
    vec4 tex = texture(texture0, uv);

    // dissolve per chunk
    float dissolve = step(progress, r);
    tex.a *= dissolve;

    // smooth fade out near the end
    tex.a *= (1.0 - progress);

    finalColor = tex;
}
