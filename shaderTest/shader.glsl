#version 330

uniform sampler2D texture0;
uniform vec2 resolution;     // screen resolution
uniform vec2 textureSize;    // original texture resolution
uniform float scale;         // zoom factor

out vec4 finalColor;

void main()
{
    vec2 frag = gl_FragCoord.xy;

    float screenAspect = resolution.x / resolution.y;
    float texAspect    = textureSize.x / textureSize.y;

    vec2 uv;

    if (screenAspect > texAspect)
    {
        // Screen wider than texture → pillarbox
        float width = resolution.y * texAspect;
        float xOffset = (resolution.x - width) * 0.5;

        uv.x = (frag.x - xOffset) / width;
        uv.y = frag.y / resolution.y;
    }
    else
    {
        // Screen taller than texture → letterbox
        float height = resolution.x / texAspect;
        float yOffset = (resolution.y - height) * 0.5;

        uv.x = frag.x / resolution.x;
        uv.y = (frag.y - yOffset) / height;
    }

    // Outside area → black
    if (uv.x < 0.0 || uv.x > 1.0 ||
        uv.y < 0.0 || uv.y > 1.0)
    {
        finalColor = vec4(0.0);
        return;
    }

    // Centered zoom
    vec2 center = vec2(0.5);
    uv = (uv - center) / scale + center;

    // Convert to texel space
    vec2 pixels = uv * textureSize;

    vec2 base = floor(pixels);
    vec2 fracPart = fract(pixels);

    vec2 aa = fwidth(pixels);

    fracPart = smoothstep(vec2(0.5) - aa,
                          vec2(0.5) + aa,
                          fracPart);

    vec2 snappedUV = (base + fracPart) / textureSize;

    finalColor = texture(texture0, snappedUV);
}

