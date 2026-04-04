#include "sound_gen.h"
#include "config.h"
#include <math.h>
#include <stdlib.h>

Wave SoundGenCreateWave(int sampleRate, float duration) {
    int samples = (int)(duration * sampleRate);
    Wave wave = {0};
    wave.frameCount = samples;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = RL_CALLOC(samples, sizeof(short));
    return wave;
}

Sound SoundGenNoiseTone(float duration, float freq, float decay) {
    int sr = AUDIO_SAMPLE_RATE;
    Wave wave = SoundGenCreateWave(sr, duration);
    int samples = wave.frameCount;
    short *d = (short *)wave.data;
    for (int i = 0; i < samples; i++) {
        float t = (float)i / sr;
        float env = expf(-t * decay);
        float noise = ((float)rand() / RAND_MAX * 2.0f - 1.0f);
        float tone = sinf(t * freq * 2.0f * PI);
        d[i] = (short)((noise * 0.6f + tone * 0.4f) * env * 28000.0f);
    }
    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

void SoundGenDegradeRadio(Wave *wav) {
    short *d = (short *)wav->data;
    int n = wav->frameCount;
    float avg = 0;
    for (int i = 0; i < n; i++) {
        float s = (float)d[i] / 32767.0f;
        avg = avg * 0.95f + s * 0.05f;
        s -= avg; // high-pass (kill bass)
        s *= AUDIO_RADIO_OVERDRIVE;
        if (s > AUDIO_RADIO_CLIP) s = AUDIO_RADIO_CLIP;
        if (s < -AUDIO_RADIO_CLIP) s = -AUDIO_RADIO_CLIP;
        s += ((float)rand() / RAND_MAX * 2.0f - 1.0f) * AUDIO_RADIO_CRACKLE;
        if (rand() % AUDIO_RADIO_POP_CHANCE == 0)
            s += ((float)rand() / RAND_MAX - 0.5f) * AUDIO_RADIO_POP_AMP;
        d[i] = (short)(s * 20000.0f);
    }
}
