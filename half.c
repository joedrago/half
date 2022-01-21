#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint16_t toHalf(uint16_t src, float scale)
{
    float mult = 1.9259299444e-34f * scale;
    float value = src * mult;
    return (uint16_t)((*(const uint32_t *)&value) >> 13);
}

float canardConvertFloat16ToNativeFloat(uint16_t value)
{
    union FP32
    {
        uint32_t u;
        float f;
    };

    const union FP32 magic = { (254UL - 15UL) << 23 };
    const union FP32 was_inf_nan = { (127UL + 16UL) << 23 };
    union FP32 out;

    out.u = (value & 0x7FFFU) << 13;
    out.f *= magic.f;
    if (out.f >= was_inf_nan.f) {
        out.u |= 255UL << 23;
    }
    out.u |= (value & 0x8000UL) << 16;

    return out.f;
}

#define MAX_DIFF 10

void runTest(uint16_t maxValue, float scale)
{
    int badCount = 0;
    int diffCounts[MAX_DIFF + 1];
    memset(diffCounts, 0, sizeof(int) * (MAX_DIFF + 1));

    for (uint16_t i = 0; i <= maxValue; ++i) {
        uint16_t h = toHalf(i, 1.0f / scale);
        float f = canardConvertFloat16ToNativeFloat(h);

        int rounded = (int)((f * scale) + 0.5f);
        int diff = rounded - i;
        if (diff < 0) {
            diff *= -1;
        }
        assert(diff <= MAX_DIFF);
        ++diffCounts[diff];

        if ((int)i != rounded) {
            ++badCount;
            // printf("BAD: [%u] becomes %f (%d)\n", i, f, rounded);
        }
    }

    printf("[0 - %u] (scale: %f): %d bad values.\n", maxValue, scale, badCount);
    for(int i = 0; i <= MAX_DIFF; ++i) {
        if(diffCounts[i] > 0) {
            printf("  * Diff[%d]: %d\n", i, diffCounts[i]);
        }
    }
}

int main(int argc, char * argv[])
{
    runTest(255, 1.0f);
    runTest(255, 255.0f);
    runTest(1023, 1.0f);
    runTest(1023, 1023.0f);
    runTest(4095, 1.0f);
    runTest(4095, 4095.0f);
    return 0;
}
