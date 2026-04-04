#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time;

out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;

    // === FISHBOWL VISOR ===
    vec2 center = uv - 0.5;
    float dist2 = dot(center, center);
    uv = 0.5 + center * (1.0 - dist2 * 0.1);
    uv = clamp(uv, 0.0, 1.0);

    // === HEAVY PIXELATION — chunky retro pixels ===
    vec2 pixelSize = vec2(320.0, 180.0);
    vec2 pixUV = floor(uv * pixelSize) / pixelSize;

    // === CHROMATIC ABERRATION ===
    float aberr = 0.002 + dist2 * 0.005;
    float r = texture(texture0, pixUV + vec2(aberr, 0.0)).r;
    float g = texture(texture0, pixUV).g;
    float b = texture(texture0, pixUV - vec2(aberr, 0.0)).b;
    vec3 col = vec3(r, g, b);

    // === THICK SCANLINES ===
    float scanline = sin(uv.y * 180.0 * 3.14159 * 2.0) * 0.07;
    col -= scanline;

    // === VISIBLE PIXEL GRID ===
    vec2 pixelGrid = fract(uv * pixelSize);
    if (pixelGrid.x < 0.1 || pixelGrid.y < 0.1) col *= 0.82;

    // === VISOR TINT ===
    col *= vec3(0.97, 1.0, 0.92);

    // === VIGNETTE ===
    float vignette = 1.0 - dist2 * 1.2;
    col *= clamp(vignette, 0.4, 1.0);

    // === FLICKER + NOISE ===
    col *= 0.96 + 0.04 * sin(time * 5.0);
    float noise = fract(sin(dot(pixUV + time * 0.1, vec2(12.9898, 78.233))) * 43758.5453);
    col += (noise - 0.5) * 0.03;

    finalColor = vec4(col, 1.0) * colDiffuse;
}
