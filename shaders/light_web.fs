#version 300 es
precision mediump float;
precision mediump int;

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform int lightCount;
uniform vec2 lightPos[128];
uniform float lightRadius;
uniform float aspect;

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

    float finalIntensity = min(0.3 + lighting, 1.0);
    finalColor = vec4(base.rgb * finalIntensity, base.a);
}
