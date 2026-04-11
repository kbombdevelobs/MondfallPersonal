#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform float time;

// Hash for pseudo-random noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    vec2 uv = fragTexCoord;

    // 1. Heavy barrel distortion (fishbowl CRT tube)
    vec2 center = uv - 0.5;
    float dist2 = dot(center, center);
    uv = 0.5 + center * (1.0 + dist2 * 0.8 + dist2 * dist2 * 0.6);

    // Discard pixels outside the tube face
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // 2. Nearest-neighbour pixelation (chunky pixel grid)
    vec2 pixelRes = vec2(120.0, 90.0);
    uv = floor(uv * pixelRes + 0.5) / pixelRes;

    // 3. Rolling interference band
    float bandPos = fract(time * 0.15);
    float bandDist = abs(uv.y - bandPos);
    float bandStrength = smoothstep(0.05, 0.0, bandDist) * 0.15;
    float bandNoise = hash(vec2(uv.x * 50.0, time * 10.0));

    // 4. Chromatic aberration (subtle)
    float caAmount = dist2 * 0.002;
    vec2 caDir = normalize(center + 0.001);
    float r = texture(texture0, uv + caDir * caAmount).r;
    float g = texture(texture0, uv).g;
    float b = texture(texture0, uv - caDir * caAmount).b;
    vec3 col = vec3(r, g, b);

    // Add interference band noise
    col += bandStrength * bandNoise;

    // 5. Interlace (every other scanline darkened)
    float scanY = uv.y * 360.0;
    float interlace = mod(floor(scanY), 2.0) == 0.0 ? 1.0 : 0.92;
    col *= interlace;

    // 6. Scanline beam profile (Gaussian bright center)
    float scanFrac = fract(scanY);
    float beam = exp(-(scanFrac - 0.5) * (scanFrac - 0.5) * 12.0);
    col *= 0.7 + 0.3 * beam;

    // 7. Phosphor stripes (subtle RGB sub-pixel tint)
    int px = int(mod(floor(uv.x * 360.0), 3.0));
    if (px == 0) col *= vec3(1.04, 0.98, 0.98);
    else if (px == 1) col *= vec3(0.98, 1.04, 0.98);
    else col *= vec3(0.98, 0.98, 1.04);

    // 8. Static grain (animated noise)
    float grain = hash(uv * 500.0 + time * 7.0) * 0.06 - 0.03;
    col += grain;

    // 9. Warm tint (mild amber)
    col *= vec3(1.03, 0.99, 0.93);

    // 10. Heavy vignette (dark edges like tube TV)
    float vig = 1.0 - dist2 * 4.0;
    vig = clamp(vig, 0.0, 1.0);
    col *= vig;

    // 11. Brightness reduction (dim old-TV feel)
    col *= 0.9;

    // 12. Flicker (subtle brightness oscillation)
    col *= 0.97 + 0.03 * sin(time * 8.0);

    finalColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}
