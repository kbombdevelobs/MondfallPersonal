#ifndef SOUND_GEN_H
#define SOUND_GEN_H

#include "raylib.h"

// Generic wave builder: allocates a 16-bit mono Wave with the given sample rate and duration
Wave SoundGenCreateWave(int sampleRate, float duration);

// Common pattern: mixed noise + tone with exponential decay envelope
Sound SoundGenNoiseTone(float duration, float freq, float decay);

// Degrade a Wave to scratchy radio quality (downsample, overdrive, crackle, pops)
// Wave should already be formatted to target sample rate before calling this.
void SoundGenDegradeRadio(Wave *wav);

#endif
