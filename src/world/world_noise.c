#include "world_noise.h"
#include "world.h"
#include <math.h>

float LerpF(float a, float b, float t) { return a + (b - a) * t; }

// Legacy hash — still used for texture generation and seeded randoms
float Hash2D(float x, float z) {
    int ix = (int)floorf(x) * 73856093;
    int iz = (int)floorf(z) * 19349663;
    int h = (ix ^ iz) & 0x7FFFFFFF;
    return (float)(h % 10000) / 10000.0f;
}

// Permutation table for gradient noise (classic Perlin)
static const unsigned char PERM[512] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

static float QuinticFade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float Grad2D(int hash, float x, float z) {
    switch (hash & 3) {
        case 0: return  x + z;
        case 1: return -x + z;
        case 2: return  x - z;
        case 3: return -x - z;
    }
    return 0;
}

float GradientNoise(float x, float z) {
    int ix = (int)floorf(x) & 255;
    int iz = (int)floorf(z) & 255;
    float fx = x - floorf(x);
    float fz = z - floorf(z);
    float u = QuinticFade(fx);
    float v = QuinticFade(fz);

    int aa = PERM[PERM[ix]     + iz];
    int ab = PERM[PERM[ix]     + iz + 1];
    int ba = PERM[PERM[ix + 1] + iz];
    int bb = PERM[PERM[ix + 1] + iz + 1];

    float l0 = LerpF(Grad2D(aa, fx, fz), Grad2D(ba, fx - 1.0f, fz), u);
    float l1 = LerpF(Grad2D(ab, fx, fz - 1.0f), Grad2D(bb, fx - 1.0f, fz - 1.0f), u);
    return LerpF(l0, l1, v);
}

// Legacy ValueNoise — still used for texture generation
static float SmoothStep(float t) { return t * t * (3.0f - 2.0f * t); }

float ValueNoise(float x, float z) {
    float ix = floorf(x), iz = floorf(z);
    float fx = x - ix, fz = z - iz;
    fx = SmoothStep(fx); fz = SmoothStep(fz);
    float a = Hash2D(ix, iz);
    float b = Hash2D(ix + 1, iz);
    float c = Hash2D(ix, iz + 1);
    float d = Hash2D(ix + 1, iz + 1);
    return LerpF(LerpF(a, b, fx), LerpF(c, d, fx), fz);
}

// Rilles — sinuous lunar channels, great for cover
static float RilleDepth(float x, float z, float seed) {
    float ca = cosf(seed), sa = sinf(seed);
    float along = x * ca + z * sa;
    float perp = -x * sa + z * ca;
    // Meandering center line
    float center = sinf(along * 0.005f + seed) * 80.0f
                 + sinf(along * 0.013f + seed * 2.3f) * 30.0f;
    float dist = fabsf(perp - center);
    float width = 6.0f + sinf(along * 0.008f + seed * 1.7f) * 2.0f;
    if (dist > width) return 0.0f;
    float t = dist / width;
    return -(1.0f - t * t) * 2.5f;
}

// Maria factor — large flat dark "sea" regions via cell noise
float WorldGetMareFactor(float x, float z) {
    float s = 0.002f;
    float nx = x * s, nz = z * s;
    float ix = floorf(nx), iz = floorf(nz);
    float minDist = 999.0f;
    for (int di = -1; di <= 1; di++) {
        for (int dj = -1; dj <= 1; dj++) {
            float cx = ix + di + Hash2D(ix + di, iz + dj);
            float cz = iz + dj + Hash2D(ix + di + 37, iz + dj + 53);
            float dx = nx - cx, dz = nz - cz;
            float d = dx * dx + dz * dz;
            if (d < minDist) minDist = d;
        }
    }
    float t = 1.0f - minDist * 4.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t; // 0 = highlands, 1 = mare center
}

// Multi-octave gradient noise for terrain with domain warping + features
float WorldGetHeight(float x, float z) {
    float s = 0.012f;
    float sx = x * s, sz = z * s;

    // Domain warp — organic, non-repetitive shapes
    float warpX = GradientNoise(sx + 5.3f, sz + 1.3f) * 4.0f;
    float warpZ = GradientNoise(sx + 9.2f, sz + 2.8f) * 4.0f;
    float wsx = sx + warpX * 0.15f;
    float wsz = sz + warpZ * 0.15f;

    // FBM with warped large octaves, unwarped fine detail
    float h = 0;
    h += GradientNoise(wsx, wsz) * 8.0f;                     // big hills (warped)
    h += GradientNoise(wsx * 2.5f, wsz * 2.5f) * 3.0f;      // ridges (warped)
    h += GradientNoise(sx * 6.0f, sz * 6.0f) * 1.0f;         // bumps (crisp)
    h += GradientNoise(sx * 15.0f, sz * 15.0f) * 0.3f;       // fine grit (crisp)

    // Rilles — two channels at different angles
    h += RilleDepth(x, z, 0.0f);
    h += RilleDepth(x, z, 1.2f);

    // Maria — flatten terrain in dark sea regions
    float mare = WorldGetMareFactor(x, z);
    if (mare > 0.05f) {
        // Pull height toward flat baseline, reduce noise in maria
        h = LerpF(h, -3.0f, mare * 0.7f);
    }

    return h - 5.0f;
}
