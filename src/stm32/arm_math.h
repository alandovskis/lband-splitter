#pragma once
#include <cstdint>

typedef float float32_t;

typedef struct {
    uint16_t fftLen;
} arm_rfft_fast_instance_f32;

inline void arm_rfft_fast_init_f32(arm_rfft_fast_instance_f32* S, uint16_t fftLen) {
    S->fftLen = fftLen;
}

inline void arm_rfft_fast_f32(const arm_rfft_fast_instance_f32*, const float32_t* src, float32_t* dst, uint8_t) {
    for (uint16_t i = 0; i < 2; ++i) {
        dst[i] = src[i];
    }
}

inline void arm_cmplx_mag_f32(const float32_t*, float32_t* dst, uint16_t len) {
    for (uint16_t i = 0; i < len; ++i) {
        dst[i] = 0.f;
    }
}

inline void arm_max_f32(const float32_t*, uint16_t, float32_t* maxVal, uint32_t* index) {
    *maxVal = 0.f;
    *index = 0;
}
