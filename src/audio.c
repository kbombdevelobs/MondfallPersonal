#include "audio.h"
#include <stdlib.h>
#include <math.h>

typedef struct { float freq; int eighths; } MN;

void GameAudioInit(GameAudio *ga) {
    int sr = 22050;

    // Erika melody from 48407_Erika.mid (Herms Niel, public domain)
    // Top note extracted per tick, filtered >= C4, 240 BPM march
    MN melody[] = {
        // Verse 1
        {415.3f,2},{311.1f,1},{311.1f,1},{311.1f,1},{311.1f,1},{261.6f,1},{311.1f,1},
        {349.2f,2},{233.1f,1},{233.1f,1},{233.1f,1},{277.2f,1},{233.1f,1},{349.2f,1},
        {311.1f,2},{311.1f,1},{311.1f,1},{311.1f,2},{311.1f,1},{311.1f,1},
        {311.1f,4},{311.1f,1},{329.6f,1},{349.2f,1},{392.0f,1},{415.3f,4},
        // Bridge
        {311.1f,4},{311.1f,4},{311.1f,4},{311.1f,4},{311.1f,4},
        {261.6f,6},{277.2f,2},{311.1f,4},{311.1f,4},{311.1f,4},
        // Verse 2
        {415.3f,4},{415.3f,4},{523.3f,4},{523.3f,6},{466.2f,2},
        {415.3f,4},{311.1f,4},{311.1f,4},{311.1f,4},
        {392.0f,4},{415.3f,4},{466.2f,4},{311.1f,4},{392.0f,4},{392.0f,4},
        {523.3f,6},{466.2f,2},{415.3f,4},{311.1f,4},{311.1f,4},{311.1f,4},
        {261.6f,6},{277.2f,2},{311.1f,4},{311.1f,4},{311.1f,4},
        // Verse 2 repeat
        {415.3f,4},{415.3f,4},{523.3f,4},{523.3f,6},{466.2f,2},
        {415.3f,4},{311.1f,4},{311.1f,4},{311.1f,4},
        {392.0f,4},{415.3f,4},{466.2f,4},{311.1f,4},{392.0f,4},{392.0f,4},
        {523.3f,6},{466.2f,2},{415.3f,4},{311.1f,4},{311.1f,4},{311.1f,4},
        // Chorus
        {311.1f,6},{415.3f,2},{392.0f,4},{392.0f,4},{392.0f,4},{392.0f,4},
        {349.2f,4},{392.0f,4},{415.3f,4},{311.1f,4},{311.1f,4},{311.1f,4},
        {392.0f,6},{415.3f,2},{466.2f,4},{466.2f,4},{466.2f,4},{466.2f,4},
        {622.3f,4},{554.4f,4},{523.3f,4},{415.3f,4},{415.3f,4},{415.3f,4},
        // Return
        {261.6f,6},{277.2f,2},{311.1f,4},{311.1f,4},{311.1f,4},
        {415.3f,4},{415.3f,4},{523.3f,4},{523.3f,6},{466.2f,2},
        {415.3f,4},{311.1f,4},{311.1f,4},{311.1f,4},
        {392.0f,4},{415.3f,4},{466.2f,4},{311.1f,4},{392.0f,4},{392.0f,4},
        {523.3f,6},{466.2f,2},{415.3f,4},
        // End rest
        {0,16},
    };
    int melodyLen = sizeof(melody) / sizeof(melody[0]);

    float eighthSec = 60.0f / 240.0f / 2.0f; // 0.125s
    float totalEighths = 0;
    for (int i = 0; i < melodyLen; i++) totalEighths += melody[i].eighths;
    float duration = totalEighths * eighthSec;
    int samples = (int)(duration * sr);

    Wave wave = {0};
    wave.frameCount = samples; wave.sampleRate = sr;
    wave.sampleSize = 16; wave.channels = 1;
    wave.data = RL_CALLOC(samples, sizeof(short));
    short *d = (short *)wave.data;

    for (int i = 0; i < samples; i++) {
        float t = (float)i / sr;
        float accum = 0, freq = 0, noteStart = 0, noteDur = 0;
        for (int n = 0; n < melodyLen; n++) {
            noteDur = melody[n].eighths * eighthSec;
            if (t < accum + noteDur) { freq = melody[n].freq; noteStart = accum; break; }
            accum += noteDur;
        }
        float noteT = t - noteStart;
        float noteEnv = 1.0f;
        if (noteDur > 0) {
            float ph = noteT / noteDur;
            noteEnv = expf(-noteT * 3.0f) * 0.5f + 0.5f;
            if (ph > 0.9f) noteEnv *= (1.0f - ph) / 0.1f;
        }
        float brass = 0;
        if (freq > 0) {
            float p1 = fmodf(t * freq, 1.0f);
            brass = (p1 < 0.42f) ? 0.5f : -0.5f;
            float p3 = fmodf(t * freq * 3.0f, 1.0f);
            brass += (p3 < 0.5f) ? 0.1f : -0.1f;
            float p5 = fmodf(t * freq * 1.5f, 1.0f);
            brass += (p5 < 0.5f) ? 0.05f : -0.05f;
            brass *= noteEnv;
        }
        float beatSec = 60.0f / 240.0f;
        float snare = 0;
        float bib = fmodf(t / beatSec, 4.0f);
        if ((bib > 1.0f && bib < 1.025f) || (bib > 3.0f && bib < 3.025f))
            snare = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.25f;
        float bass = 0;
        float bdPh = fmodf(t / beatSec, 2.0f);
        if (bdPh < 0.018f)
            bass = sinf(bdPh * 80.0f * 6.283f) * expf(-bdPh * 70.0f) * 0.35f;
        float stat = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * 0.02f;
        float crackle = (rand() % 8000 == 0) ? ((float)rand() / RAND_MAX - 0.5f) * 0.15f : 0;
        float mix = brass * 0.38f + snare * 0.12f + bass * 0.15f + stat + crackle;
        if (mix > 0.95f) mix = 0.95f;
        if (mix < -0.95f) mix = -0.95f;
        d[i] = (short)(mix * 14000.0f);
    }

    ExportWave(wave, "assets/march.wav");
    UnloadWave(wave);
    ga->marchMusic = LoadMusicStream("assets/march.wav");
    ga->marchMusic.looping = true;
    SetMusicVolume(ga->marchMusic, 0.3f);
    PlayMusicStream(ga->marchMusic);
    ga->loaded = true;
}

void GameAudioUpdate(GameAudio *ga) {
    if (ga->loaded) UpdateMusicStream(ga->marchMusic);
}

void GameAudioSetMusicVolume(GameAudio *ga, float vol) {
    if (ga->loaded) SetMusicVolume(ga->marchMusic, vol);
}

void GameAudioUnload(GameAudio *ga) {
    if (ga->loaded) UnloadMusicStream(ga->marchMusic);
}
