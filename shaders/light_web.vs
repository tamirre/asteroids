#version 300 es
precision mediump float;
precision mediump int;

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;   // MUST exist

uniform mat4 mvp;

out vec2 fragTexCoord;

void main() {
    fragTexCoord = vertexTexCoord;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
