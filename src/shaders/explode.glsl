#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D texture0;
uniform float progress;

void main()
{
    // texture size
    vec2 texSize = vec2(textureSize(texture0, 0));
    float chunkSize = 4.0;
    vec2 grid = floor(fragTexCoord * texSize / chunkSize);
    vec2 chunkUV = grid * chunkSize / texSize;
    vec2 dir = vec2(0.0, 1.0);

    // --- hardcoded explosion strength ---
    float strength = 0.005;
    float move = progress * strength;
    vec2 uv = chunkUV - dir * move;

    // sample texture
    vec4 tex = texture(texture0, uv);

    // smooth fade out near the end
    tex.a *= (1.0 - progress);

    finalColor = tex;
}
