#version 100
precision mediump float;
precision mediump int;

varying vec2 fragTexCoord;

uniform sampler2D texture0;
uniform int lightCount;
uniform vec2 lightPos[128];
uniform float lightRadius;
uniform float aspect;

void main()
{
    vec4 base = texture2D(texture0, fragTexCoord);

    float lighting = 0.0;

    for (int i = 0; i < 128; i++)
    {
        if (i >= lightCount) break;

        vec2 p = fragTexCoord;
        p.x *= aspect;

        vec2 lp = lightPos[i];
        lp.x *= aspect;

        float dist = distance(p, lp);
        float intensity = 1.0 - smoothstep(0.0, lightRadius, dist);

        lighting += intensity * 1.7;
    }

    float finalIntensity = min(0.3 + lighting, 1.0);

    gl_FragColor = vec4(base.rgb * finalIntensity, base.a);
}

