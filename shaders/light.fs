#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;

// --- CHANGED: array of lights instead of one ---
uniform int lightCount;
uniform vec2 lightPos[128];     // normalized 0–1
// -----------------------------------------------

uniform float lightRadius;      // normalized
uniform float aspect;           // screenWidth / screenHeight

void main()
{
    vec4 base = texture(texture0, fragTexCoord);

    float lighting = 0.0;

    for (int i = 0; i < lightCount; i++)
    {
        // Apply aspect fix like before
        vec2 p = fragTexCoord;
        p.x *= aspect;

        vec2 lp = lightPos[i];
        lp.x *= aspect;

        float dist = distance(p, lp);
        float intensity = 1.0 - smoothstep(0.0, lightRadius, dist);

        // Accumulate — same formula as original
        lighting += intensity * 1.7;
    }

    // Base ambient 0.3 stays constant
    float finalIntensity = 0.3 + lighting;

    // Clamp so lights don’t blow out white
    finalIntensity = min(finalIntensity, 1.0);

    finalColor = vec4(base.rgb * finalIntensity, base.a);
}

