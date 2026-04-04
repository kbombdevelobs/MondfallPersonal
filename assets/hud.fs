#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;

    // Fishbowl bend — edges pinch inward (inside a helmet)
    vec2 center = uv - 0.5;
    float dist2 = dot(center, center);
    uv = 0.5 + center * (1.0 - dist2 * 0.12);

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0, 0, 0, 0);
        return;
    }

    // NO pixelation — crisp text
    vec4 col = texture(texture0, uv);

    // Slight glow on bright HUD elements
    if (col.a > 0.1) {
        col.rgb *= 1.05;
    }

    finalColor = col * colDiffuse;
}
