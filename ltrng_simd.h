#ifndef LTRNG_SIMD
#define LTRNG_SIMD


#include <immintrin.h>
#include <stdint.h>

#include "./ltrng.h"

struct ltrng_simd_State
{
    __m512i seedLo;
    __m512i seedHi;
    struct ltrng_Seed hash;
};

__m512i ltrng_simd_util_mix_stafford_13(__m512i seed);
__m512i ltrng_simd_util_rotl(__m512i v, int8_t r);
__m512i ltrng_simd_util_mask_rotl(__m512i v, int8_t r, __mmask8 mask);

void ltrng_simd_state_init(struct ltrng_simd_State *state, const char *str);
void ltrng_simd_state_set_seeds(struct ltrng_simd_State *state, __m512i seeds);
void ltrng_simd_state_mask_set_seed(struct ltrng_simd_State *state, int64_t seed, __mmask8 mask);

__m512i ltrng_simd_state_next_long(struct ltrng_simd_State *state);
__m512i ltrng_simd_state_next_int(struct ltrng_simd_State *state);
__m512i ltrng_simd_state_next_int_range(struct ltrng_simd_State *state, int32_t i);

__m512i ltrng_simd_state_mask_next_long(struct ltrng_simd_State *state, __mmask8 mask);
__m512i ltrng_simd_state_mask_next_int(struct ltrng_simd_State *state, __mmask8 mask);

#endif