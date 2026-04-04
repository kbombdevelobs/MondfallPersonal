#ifndef AUDIO_H
#define AUDIO_H

#include "raylib.h"

typedef struct {
    Music marchMusic;
    bool loaded;
} GameAudio;

void GameAudioInit(GameAudio *ga);
void GameAudioUpdate(GameAudio *ga);
void GameAudioSetMusicVolume(GameAudio *ga, float vol);
void GameAudioUnload(GameAudio *ga);

#endif
