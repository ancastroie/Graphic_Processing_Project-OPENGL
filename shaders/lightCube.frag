#version 410 core

in vec2 UV; // UV from vertex shader
in vec3 Normal; // Normal from vertex shader
in vec3 FragPos; // Position from vertex shader

out vec4 fColor; // Output color of the pixel

uniform sampler2D diffuseTexture; // Texture with alpha channel

void main() {
    vec4 colorFromTexture = texture(diffuseTexture, UV);

    // Discard fragment if alpha is less than 0.1
    if (colorFromTexture.a < 0.1) {
        discard;
    }

    fColor = colorFromTexture;
}
