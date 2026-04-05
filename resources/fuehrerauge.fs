#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time;

out vec4 finalColor;

// Hash for noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 center = uv - 0.5;
    float dist2 = dot(center, center);

    // Heavy fishbowl barrel distortion
    uv = 0.5 + center * (1.0 + dist2 * 0.8 + dist2 * dist2 * 0.6);

    // Out of bounds — black
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Nearest-neighbour pixelation — coarse
    vec2 texSize = vec2(120.0, 90.0);
    uv = floor(uv * texSize + 0.5) / texSize;

    // Rolling horizontal band of noise (like old TV interference)
    float rollY = fract(time * 0.15);
    float bandDist = abs(fragTexCoord.y - rollY);
    if (bandDist > 0.5) bandDist = 1.0 - bandDist; // wrap
    float rollNoise = smoothstep(0.06, 0.0, bandDist) * 0.3;

    // Minimal chromatic aberration — just a hint
    float aberr = dist2 * 0.002;
    vec2 dir = normalize(center + 0.001) * aberr;
    float r = texture(texture0, uv + dir).r;
    float g = texture(texture0, uv).g;
    float b = texture(texture0, uv - dir).b;

    vec3 col = vec3(r, g, b);

    // Subtle rolling noise band
    col += rollNoise * vec3(0.4, 0.35, 0.25);

    // Interlace lines — every other scanline darkened
    float interlace = mod(floor(fragTexCoord.y * 360.0), 2.0);
    col *= 0.85 + 0.15 * interlace;

    // Thick scanline beam profile
    float scanY = fract(fragTexCoord.y * 180.0);
    float scanDist = scanY - 0.5;
    float scanBeam = 0.7 + 0.3 * exp(-scanDist * scanDist * 12.0);
    col *= scanBeam;

    // Subtle phosphor stripe (very mild)
    int px = int(mod(floor(fragTexCoord.x * 360.0), 3.0));
    if (px == 0) col *= vec3(1.05, 0.97, 0.97);
    else if (px == 1) col *= vec3(0.97, 1.05, 0.97);
    else col *= vec3(0.97, 0.97, 1.05);

    // Light static grain
    float grain = hash(fragTexCoord * 500.0 + vec2(time * 7.0, time * 11.0));
    col += (grain - 0.5) * 0.04;

    // Mild warm tint — keep colors mostly faithful
    col *= vec3(1.03, 0.99, 0.93);

    // Heavy vignette — dark edges like an old tube
    float vig = 1.0 - dist2 * 4.0;
    col *= clamp(vig, 0.1, 1.0);

    // Overall brightness reduction for that dim old TV look
    col *= 0.9;

    // Flicker
    col *= 0.97 + 0.03 * sin(time * 8.0);

    finalColor = vec4(clamp(col, 0.0, 1.0), 1.0) * colDiffuse;
}
