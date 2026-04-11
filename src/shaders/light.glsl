#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;

uniform int lightCount;
uniform float ambience;
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
        vec2 p = fragTexCoord;
        p.x *= aspect;

        vec2 lp = lightPos[i];
        lp.x *= aspect;

        float dist = distance(p, lp);
        float intensity = 1.0 - smoothstep(0.0, lightRadius, dist);
        lighting += intensity * 1.7;
    }


	// base ambeint is 0.5
    float finalIntensity = ambience + lighting;

    // Clamp so lights don’t blow out white
    finalIntensity = min(finalIntensity, 1.0);

    finalColor = vec4(base.rgb * finalIntensity, base.a);
}

