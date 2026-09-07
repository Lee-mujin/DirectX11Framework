#pragma once
#include <vector>

class PerlinNoise
{
public:
    PerlinNoise(unsigned int seed = 1337);

    // Basic 2D perlin noise returning value roughly in [-1, 1]
    float Noise2D(float x, float y) const;

    // Multi-octave Fractal Brownian Motion (fBm) noise
    float OctaveNoise2D(float x, float y, int octaves = 4, float persistence = 0.5f, float lacunarity = 2.0f) const;

private:
    float Fade(float t) const { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
    float Lerp(float a, float b, float t) const { return a + t * (b - a); }
    float Grad(int hash, float x, float y) const;

    std::vector<int> m_permutation;
};