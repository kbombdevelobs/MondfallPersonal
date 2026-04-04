#ifndef WORLD_NOISE_H
#define WORLD_NOISE_H

float LerpF(float a, float b, float t);
float Hash2D(float x, float z);
float GradientNoise(float x, float z);
float ValueNoise(float x, float z);

#endif
