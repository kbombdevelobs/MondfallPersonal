#include "audio.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// WaveToMemoryWav: encode a raylib Wave into a minimal RIFF WAV byte buffer.
// Caller owns the returned buffer and must free() it.
// ---------------------------------------------------------------------------
static unsigned char *WaveToMemoryWav(const Wave *w, int *outSize) {
    int dataBytes = w->frameCount * w->channels * (w->sampleSize / 8);
    int totalSize = 44 + dataBytes;
    unsigned char *buf = (unsigned char *)RL_MALLOC(totalSize);
    if (!buf) { *outSize = 0; return NULL; }

    // RIFF header
    memcpy(buf +  0, "RIFF", 4);
    int chunkSize = totalSize - 8;
    memcpy(buf +  4, &chunkSize, 4);
    memcpy(buf +  8, "WAVE", 4);

    // fmt sub-chunk
    memcpy(buf + 12, "fmt ", 4);
    int fmtSize = 16;
    memcpy(buf + 16, &fmtSize, 4);
    short audioFormat = 1; // PCM
    memcpy(buf + 20, &audioFormat, 2);
    short channels = (short)w->channels;
    memcpy(buf + 22, &channels, 2);
    int sampleRate = w->sampleRate;
    memcpy(buf + 24, &sampleRate, 4);
    int byteRate = sampleRate * w->channels * (w->sampleSize / 8);
    memcpy(buf + 28, &byteRate, 4);
    short blockAlign = (short)(w->channels * (w->sampleSize / 8));
    memcpy(buf + 32, &blockAlign, 2);
    short bitsPerSample = (short)w->sampleSize;
    memcpy(buf + 34, &bitsPerSample, 2);

    // data sub-chunk
    memcpy(buf + 36, "data", 4);
    memcpy(buf + 40, &dataBytes, 4);
    memcpy(buf + 44, w->data, dataBytes);

    *outSize = totalSize;
    return buf;
}

// ---------------------------------------------------------------------------
// Shared brass-radio synthesis: converts an MN melody array into a Wave
// Produces square-wave brass with 3rd/5th harmonics, snare on 2&4, bass drum
// on 1&3, plus radio static and crackle overlay.  240 BPM march tempo.
// ---------------------------------------------------------------------------
static Wave SynthesizeMarch(const MN *melody, int melodyLen) {
    int sr = 22050;
    int bpm = 240;
    float eighthSec = 60.0f / (float)bpm / 2.0f; // 0.125s

    // Total duration in samples
    float totalEighths = 0;
    for (int i = 0; i < melodyLen; i++) totalEighths += melody[i].eighths;
    float duration = totalEighths * eighthSec;
    int samples = (int)(duration * (float)sr);
    if (samples < 1) samples = 1;

    Wave wave = {0};
    wave.frameCount = samples;
    wave.sampleRate = sr;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = RL_CALLOC(samples, sizeof(short));
    short *d = (short *)wave.data;

    // Fill note-by-note (O(samples) instead of O(samples * melodyLen))
    int samplePos = 0;
    for (int n = 0; n < melodyLen; n++) {
        float freq = melody[n].freq;
        float noteDur = melody[n].eighths * eighthSec;
        int noteSamples = (int)(noteDur * (float)sr);

        for (int i = 0; i < noteSamples && samplePos < samples; i++, samplePos++) {
            float t = (float)samplePos / (float)sr;
            float noteT = (float)i / (float)sr;

            // Note envelope
            float noteEnv = 1.0f;
            if (noteDur > 0.001f) {
                float ph = noteT / noteDur;
                noteEnv = expf(-noteT * 3.0f) * 0.5f + 0.5f;
                if (ph > 0.9f) noteEnv *= (1.0f - ph) / 0.1f;
            }

            // Brass square-wave with harmonics
            float brass = 0;
            if (freq > 0.0f) {
                float p1 = fmodf(t * freq, 1.0f);
                brass = (p1 < 0.42f) ? 0.5f : -0.5f;
                float p3 = fmodf(t * freq * 3.0f, 1.0f);
                brass += (p3 < 0.5f) ? 0.1f : -0.1f;
                float p5 = fmodf(t * freq * 1.5f, 1.0f);
                brass += (p5 < 0.5f) ? 0.05f : -0.05f;
                brass *= noteEnv;
            }

            // Snare on beats 2 and 4
            float beatSec = 60.0f / (float)bpm;
            float bib = fmodf(t / beatSec, 4.0f);
            float snare = 0;
            if ((bib > 1.0f && bib < 1.025f) || (bib > 3.0f && bib < 3.025f))
                snare = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.25f;

            // Bass drum on beats 1 and 3
            float bass = 0;
            float bdPh = fmodf(t / beatSec, 2.0f);
            if (bdPh < 0.018f)
                bass = sinf(bdPh * 80.0f * 6.283f) * expf(-bdPh * 70.0f) * 0.35f;

            // Radio static + crackle
            float stat = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.02f;
            float crackle = (rand() % 8000 == 0)
                ? ((float)rand() / RAND_MAX - 0.5f) * 0.15f : 0.0f;

            float mix = brass * 0.38f + snare * 0.12f + bass * 0.15f + stat + crackle;
            if (mix >  0.95f) mix =  0.95f;
            if (mix < -0.95f) mix = -0.95f;
            d[samplePos] = (short)(mix * 14000.0f);
        }
    }

    return wave;
}

// ---------------------------------------------------------------------------
// Pick a random march index different from the current one
// ---------------------------------------------------------------------------
static int PickNextMarch(int currentIndex) {
    if (MARCH_COUNT <= 1) return 0;
    int next;
    do {
        next = rand() % MARCH_COUNT;
    } while (next == currentIndex);
    return next;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void GameAudioInit(GameAudio *ga) {
    if (!ga) return;
    memset(ga, 0, sizeof(*ga));

    // Synthesize and load every march (in-memory, no filesystem writes)
    for (int i = 0; i < MARCH_COUNT; i++) {
        const MarchDef *def = &g_marches[i];

        Wave wave = SynthesizeMarch(def->notes, def->noteCount);

        // Encode to WAV in memory and stream from buffer
        int wavSize = 0;
        unsigned char *wavBuf = WaveToMemoryWav(&wave, &wavSize);
        UnloadWave(wave);

        ga->wavBuffers[i] = wavBuf;
        ga->wavBufferSizes[i] = wavSize;

        if (wavBuf && wavSize > 0) {
            ga->streams[i] = LoadMusicStreamFromMemory(".wav", wavBuf, wavSize);
        }
        ga->streams[i].looping = false;  // we handle advancement ourselves
    }

    // Always start with Erika (index 0)
    ga->currentIndex = 0;
    ga->volume = 0.3f;
    SetMusicVolume(ga->streams[0], ga->volume);
    PlayMusicStream(ga->streams[0]);
    ga->loaded = true;
}

void GameAudioUpdate(GameAudio *ga) {
    if (!ga->loaded) return;

    int idx = ga->currentIndex;
    UpdateMusicStream(ga->streams[idx]);

    // When the current march finishes, pick a random next one
    if (!IsMusicStreamPlaying(ga->streams[idx])) {
        StopMusicStream(ga->streams[idx]);

        int next = PickNextMarch(idx);
        ga->currentIndex = next;

        SetMusicVolume(ga->streams[next], ga->volume);
        PlayMusicStream(ga->streams[next]);
    }
}

void GameAudioSetMusicVolume(GameAudio *ga, float vol) {
    if (!ga->loaded) return;
    ga->volume = vol;
    SetMusicVolume(ga->streams[ga->currentIndex], vol);
}

const char *GameAudioGetCurrentName(GameAudio *ga) {
    if (!ga->loaded) return "";
    return g_marches[ga->currentIndex].name;
}

void GameAudioSkip(GameAudio *ga) {
    if (!ga->loaded) return;
    int idx = ga->currentIndex;
    StopMusicStream(ga->streams[idx]);
    int next = PickNextMarch(idx);
    ga->currentIndex = next;
    SetMusicVolume(ga->streams[next], ga->volume);
    PlayMusicStream(ga->streams[next]);
}

void GameAudioUnload(GameAudio *ga) {
    if (!ga->loaded) return;
    for (int i = 0; i < MARCH_COUNT; i++) {
        UnloadMusicStream(ga->streams[i]);
        if (ga->wavBuffers[i]) {
            RL_FREE(ga->wavBuffers[i]);
            ga->wavBuffers[i] = NULL;
        }
    }
    ga->loaded = false;
}
