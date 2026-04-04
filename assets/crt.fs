#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time;

out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord;

    // === FISHBOWL VISOR DISTORTION ===
    vec2 center = uv - 0.5;
    float dist2 = dot(center, center);
    uv = 0.5 + center * (1.0 - dist2 * 0.1);
    uv = clamp(uv, 0.0, 1.0);

    // === PIXELATION ===
    vec2 pixelSize = vec2(320.0, 180.0);
    vec2 pixUV = floor(uv * pixelSize) / pixelSize;
    vec2 pixCenter = pixUV + 0.5 / pixelSize; // center of the pixel cell

    // === RADIAL CHROMATIC ABERRATION (stronger at visor edges) ===
    float aberr = 0.001 + dist2 * 0.006;
    vec2 aberrDir = normalize(center) * aberr; // radial, not just horizontal
    float r = texture(texture0, pixUV + aberrDir).r;
    float g = texture(texture0, pixUV).g;
    float b = texture(texture0, pixUV - aberrDir).b;
    vec3 col = vec3(r, g, b);

    // === FAKE SINGLE-PASS BLOOM (5-tap cross sample) ===
    vec2 texel = 1.0 / pixelSize;
    vec3 bloom = vec3(0.0);
    bloom += max(texture(texture0, pixUV + vec2(texel.x, 0)).rgb - 0.5, 0.0);
    bloom += max(texture(texture0, pixUV - vec2(texel.x, 0)).rgb - 0.5, 0.0);
    bloom += max(texture(texture0, pixUV + vec2(0, texel.y)).rgb - 0.5, 0.0);
    bloom += max(texture(texture0, pixUV - vec2(0, texel.y)).rgb - 0.5, 0.0);
    bloom += max(texture(texture0, pixUV + vec2(texel.x, texel.y)).rgb - 0.5, 0.0);
    bloom += max(texture(texture0, pixUV - vec2(texel.x, texel.y)).rgb - 0.5, 0.0);
    col += bloom * 0.08; // subtle warm glow on bright areas

    // === GAUSSIAN SCANLINE BEAM ===
    float scanY = uv.y * 180.0;
    float scanDist = fract(scanY) - 0.5; // distance from scanline center
    float scanBeam = exp(-scanDist * scanDist * 18.0); // Gaussian beam profile
    col *= mix(0.75, 1.0, scanBeam); // dark gaps between scanlines

    // === APERTURE GRILLE PHOSPHOR MASK (Trinitron-style RGB stripes) ===
    int subpx = int(mod(gl_FragCoord.x, 3.0));
    vec3 mask = vec3(0.8);
    if (subpx == 0) mask.r = 1.0;
    else if (subpx == 1) mask.g = 1.0;
    else mask.b = 1.0;
    col *= mix(vec3(1.0), mask, 0.25); // subtle phosphor tint

    // === PIXEL GRID ===
    vec2 pixelGrid = fract(uv * pixelSize);
    float grid = step(0.08, pixelGrid.x) * step(0.08, pixelGrid.y);
    col *= mix(0.82, 1.0, grid);

    // Visor tint — slight gold/green
    col *= vec3(0.97, 1.0, 0.92);
    float luma = dot(col, vec3(0.299, 0.587, 0.114)); // needed for grain

    // === FAINT GHOST REFLECTION (internal visor bounce) ===
    vec2 ghostUV = vec2(1.0 - pixUV.x, pixUV.y); // horizontally mirrored
    vec3 ghost = texture(texture0, ghostUV).rgb;
    col += ghost * 0.025; // very faint reflection

    // === ELLIPTICAL VIGNETTE (helmet-shaped, wider than tall) ===
    vec2 vigUV = (uv - 0.5) * vec2(1.3, 1.6); // stretched vertically
    float vig = 1.0 - dot(vigUV, vigUV);
    col *= clamp(vig, 0.15, 1.0);

    // === LUMINANCE-DEPENDENT FILM GRAIN ===
    // Organic grain — stronger in shadows, colored per-channel
    float grain_r = fract(sin(dot(pixUV + time * 0.13, vec2(12.9898, 78.233))) * 43758.5453);
    float grain_g = fract(sin(dot(pixUV + time * 0.17, vec2(93.9898, 67.345))) * 28461.2534);
    float grain_b = fract(sin(dot(pixUV + time * 0.11, vec2(45.1234, 94.567))) * 51732.8765);
    float grainStrength = 0.04 * (1.0 - luma); // more grain in dark areas
    col.r += (grain_r - 0.5) * grainStrength;
    col.g += (grain_g - 0.5) * grainStrength;
    col.b += (grain_b - 0.5) * grainStrength;

    // === SUBTLE BREATH FOG (bottom of visor) ===
    float fogY = smoothstep(0.0, 0.15, uv.y); // bottom 15% of screen
    float fogNoise = fract(sin(dot(uv * 4.0 + time * 0.3, vec2(23.14, 45.89))) * 12345.6789);
    float fog = (1.0 - fogY) * 0.08 * (0.7 + fogNoise * 0.3);
    fog *= 0.7 + sin(time * 0.8) * 0.3; // slow breathing pulse
    col = mix(col, vec3(0.85, 0.88, 0.9), fog);

    // === FLICKER ===
    col *= 0.97 + 0.03 * sin(time * 5.0);

    finalColor = vec4(clamp(col, 0.0, 1.0), 1.0) * colDiffuse;
}
