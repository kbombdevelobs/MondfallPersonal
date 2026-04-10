#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"
#include "audio_marches.h"

typedef struct {
    Music   streams[MARCH_COUNT];   // one Music per march
    int     currentIndex;           // which march is playing now
    float   volume;                 // tracked separately (no GetMusicVolume in raylib)
    bool    loaded;
    unsigned char *wavBuffers[MARCH_COUNT]; // in-memory WAV data for LoadMusicStreamFromMemory
    int wavBufferSizes[MARCH_COUNT];        // byte sizes of wavBuffers[]
} GameAudio;

/// Synthesize all march WAVs from MIDI note data, load them as music streams, and start playback.
void GameAudioInit(GameAudio *ga);
/// Update the current music stream and auto-advance to a random march when the current one finishes.
void GameAudioUpdate(GameAudio *ga);
/// Set the music playback volume for the currently playing march stream.
void GameAudioSetMusicVolume(GameAudio *ga, float vol);
/// Unload all music streams and mark the audio system as unloaded.
void GameAudioUnload(GameAudio *ga);
/// Return the display name of the currently playing march (empty string if not loaded).
const char *GameAudioGetCurrentName(GameAudio *ga);
/// Stop the current march and immediately start playing a different random march.
void GameAudioSkip(GameAudio *ga);

#endif
