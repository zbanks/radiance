#include "perlin.h"
#include <stdlib.h>
#include <math.h>

// Permutation lookup table
int p[PERLIN_PERIOD];

// Random unit-vector gradient table
float gx[PERLIN_PERIOD];
float gy[PERLIN_PERIOD];
float gz[PERLIN_PERIOD];

void perlin_init()
{
    for(int i = 0; i < PERLIN_PERIOD; i++)
    {
        p[i] = i;
    }

    for(int i = 0; i < PERLIN_PERIOD - 1; i++)
    {
      int j = i + rand() / (RAND_MAX / (PERLIN_PERIOD - i) + 1);
      int t = p[j];
      p[j] = p[i];
      p[i] = t;
    }

    for(int i = 0; i < PERLIN_PERIOD; i++)
    {
        float x;
        float y;
        float z;
        do
        {
            x = 2 * (float)rand() / RAND_MAX - 1;
            y = 2 * (float)rand() / RAND_MAX - 1;
            z = 2 * (float)rand() / RAND_MAX - 1;
        } while(x*x + y*y + z*z > 1);
        float mag = sqrt(x*x + y*y + z*z);
        gx[i] = x / mag;
        gy[i] = y / mag;
        gz[i] = z / mag;
    }
}

static int hash(long x, long y, long z)
{
    int xi = ((x) % PERLIN_PERIOD + PERLIN_PERIOD) % PERLIN_PERIOD;
    int yi = ((y + p[xi]) % PERLIN_PERIOD + PERLIN_PERIOD) % PERLIN_PERIOD;
    int zi = ((z + p[yi]) % PERLIN_PERIOD + PERLIN_PERIOD) % PERLIN_PERIOD;

    return p[zi];
}

static float cross_fade(float t)
{
    return (3 - 2 * t) * t * t;
}

static float contribution(float px, float py, float pz, float qx, float qy, float qz)
{
    int h = hash((long)qx, (long)qy, (long)qz);
    return (gx[h] * (px - qx) + gy[h] * (py - qy) + gz[h] * (pz - qz))
            * cross_fade(1 - fabs(px - qx))
            * cross_fade(1 - fabs(py - qy))
            * cross_fade(1 - fabs(pz - qz));
}

float perlin3d(float x, float y, float z)
{
    float xi = floor(x);
    float yi = floor(y);
    float zi = floor(z);

    float result =
        contribution(x, y, z, xi,   yi,   zi  ) +
        contribution(x, y, z, xi+1, yi,   zi  ) +
        contribution(x, y, z, xi,   yi+1, zi  ) +
        contribution(x, y, z, xi+1, yi+1, zi  ) +
        contribution(x, y, z, xi,   yi,   zi+1) +
        contribution(x, y, z, xi+1, yi,   zi+1) +
        contribution(x, y, z, xi,   yi+1, zi+1) +
        contribution(x, y, z, xi+1, yi+1, zi+1);

    result = (result + 0.5);

    return result;
}

