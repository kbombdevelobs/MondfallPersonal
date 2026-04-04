#include "weapon.h"
#include "weapon_sound.h"
#include "sound_gen.h"
#include <math.h>
#include <stdlib.h>

Sound GenSoundGunshot(void) {
    return SoundGenNoiseTone(0.15f, 120.0f, 25.0f);
}

Sound GenSoundBeamBlast(void) {
    // 2.5 second sustained energy beam roar
    int sr = AUDIO_SAMPLE_RATE;
    Wave wave = SoundGenCreateWave(sr, 2.5f);
    short *d = (short *)wave.data;
    for (int i = 0; i < (int)wave.frameCount; i++) {
        float t = (float)i / sr;
        // Ramp up then sustain then fade
        float env = 1.0f;
        if (t < 0.1f) env = t / 0.1f; // ramp up
        if (t > 2.0f) env = (2.5f - t) / 0.5f; // fade out
        // Deep rumble + mid screech + high sizzle
        float rumble = sinf(t * 55.0f * 2.0f * PI) * 0.4f;
        float screech = sinf(t * 280.0f * 2.0f * PI + sinf(t * 8.0f) * 3.0f) * 0.3f;
        float sizzle = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.25f;
        float pulse = 1.0f + sinf(t * 12.0f) * 0.15f; // pulsing intensity
        d[i] = (short)((rumble + screech + sizzle) * env * pulse * 22000.0f);
    }
    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

Sound GenSoundJackhammer(void) {
    // Brutal pneumatic crunch + metallic impact + servo whine
    int sr = AUDIO_SAMPLE_RATE;
    Wave wave = SoundGenCreateWave(sr, 0.35f);
    short *d = (short *)wave.data;
    for (int i = 0; i < (int)wave.frameCount; i++) {
        float t = (float)i / sr;
        float env = (t < 0.02f) ? t / 0.02f : expf(-(t - 0.02f) * 8.0f);
        // Heavy impact thud
        float thud = sinf(t * 80.0f * 2.0f * PI) * expf(-t * 12.0f) * 0.5f;
        // Metallic crunch noise
        float crunch = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * expf(-t * 6.0f) * 0.4f;
        // Servo whine — rising pitch
        float whine = sinf(t * (400.0f + t * 2000.0f) * 2.0f * PI) * 0.15f * expf(-t * 4.0f);
        // Pneumatic hiss
        float hiss = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.2f * expf(-t * 10.0f);
        d[i] = (short)((thud + crunch + whine + hiss) * env * 28000.0f);
    }
    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

Sound GenSoundReload(void) {
    int sr = AUDIO_SAMPLE_RATE;
    Wave wave = SoundGenCreateWave(sr, 0.5f);
    short *d = (short *)wave.data;
    for (int i = 0; i < (int)wave.frameCount; i++) {
        float t = (float)i / sr;
        float click1 = (t > 0.05f && t < 0.08f) ? sinf(t * 3000) * 0.8f : 0;
        float click2 = (t > 0.25f && t < 0.3f) ? sinf(t * 2000) * 0.6f : 0;
        float slide = (t > 0.1f && t < 0.2f) ? ((float)rand()/RAND_MAX - 0.5f) * 0.3f : 0;
        d[i] = (short)((click1 + click2 + slide) * 20000.0f);
    }
    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

Sound GenSoundExplosion(void) {
    int sr = AUDIO_SAMPLE_RATE;
    Wave wave = SoundGenCreateWave(sr, 0.6f);
    short *d = (short *)wave.data;
    for (int i = 0; i < (int)wave.frameCount; i++) {
        float t = (float)i / sr;
        float env = expf(-t * 5.0f);
        float boom = sinf(t * 50.0f * 2.0f * PI) * expf(-t * 8.0f);
        float noise = ((float)rand() / RAND_MAX * 2.0f - 1.0f);
        float rumble = sinf(t * 25.0f * 2.0f * PI) * expf(-t * 3.0f);
        d[i] = (short)((boom * 0.4f + noise * 0.3f * env + rumble * 0.3f) * 30000.0f);
    }
    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

Sound GenSoundEmpty(void) {
    int sr = AUDIO_SAMPLE_RATE;
    Wave wave = SoundGenCreateWave(sr, 0.05f);
    short *d = (short *)wave.data;
    for (int i = 0; i < (int)wave.frameCount; i++) {
        float t = (float)i / sr;
        d[i] = (short)(sinf(t * 4000) * expf(-t * 80) * 15000.0f);
    }
    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}
