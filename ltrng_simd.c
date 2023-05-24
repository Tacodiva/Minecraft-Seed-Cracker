#include <stdint.h>
#include <openssl/evp.h>
#include <string.h>

#include "./ltrng_simd.h"

__m512i ltrng_simd_util_mix_stafford_13(__m512i seed)
{
    // seed = (seed ^ seed >> 30) * -4658895280553007687L;
    seed = _mm512_xor_epi64(seed, _mm512_srli_epi64(seed, 30));
    seed = _mm512_mullo_epi64(seed, _mm512_set1_epi64(-4658895280553007687L));

    // seed = (seed ^ seed >> 27) * -7723592293110705685L;
    seed = _mm512_xor_epi64(seed, _mm512_srli_epi64(seed, 27));
    seed = _mm512_mullo_epi64(seed, _mm512_set1_epi64(-7723592293110705685L));

    // return seed ^ seed >> 31;
    return _mm512_xor_epi64(seed, _mm512_srli_epi64(seed, 31));
}

__m512i ltrng_simd_util_rotl(__m512i v, int8_t r)
{
    const int bit_width = 64;
    __m512i right = _mm512_slli_epi64(v, r);
    __m512i left = _mm512_srli_epi64(v, bit_width - r);
    return _mm512_or_epi64(right, left);
}

__m512i ltrng_simd_util_mask_rotl(__m512i v, int8_t r, __mmask8 mask)
{
    const int bit_width = 64;
    __m512i right = _mm512_mask_slli_epi64(v, mask, v, r);
    __m512i left = _mm512_mask_srli_epi64(v, mask, v, bit_width - r);
    return _mm512_or_epi64(right, left);
}

void ltrng_simd_state_init(struct ltrng_simd_State *state, const char *str)
{
    EVP_MD_CTX *md5_context;
    md5_context = EVP_MD_CTX_new();
    EVP_DigestInit_ex(md5_context, EVP_md5(), NULL);
    EVP_DigestUpdate(md5_context, str, strlen(str));
    EVP_DigestFinal_ex(md5_context, (void *)&state->hash.low, NULL);

    state->hash.low = __bswap_64(state->hash.low);
    state->hash.high = __bswap_64(state->hash.high);

    EVP_MD_CTX_free(md5_context);
}

void ltrng_simd_state_set_seeds(struct ltrng_simd_State *state, __m512i seeds)
{
    // long l_seed = seed ^ 0x6A09E667F3BCC909L;
    __m512i l_seed = _mm512_xor_epi64(seeds, _mm512_set1_epi64(0x6A09E667F3BCC909L));

    // state->seed.low = ltrng_util_mix_stafford_13(l_seed ^ state->hash.low);
    state->seedLo = ltrng_simd_util_mix_stafford_13(_mm512_xor_epi64(l_seed, _mm512_set1_epi64(state->hash.low)));

    // state->seed.high = ltrng_util_mix_stafford_13((l_seed + -7046029254386353131L) ^ state->hash.high);
    state->seedHi = ltrng_simd_util_mix_stafford_13(_mm512_xor_epi64(_mm512_add_epi64(l_seed, _mm512_set1_epi64(-7046029254386353131L)), _mm512_set1_epi64(state->hash.high)));

    //  if ((s_seed.low | s_seed.high) == 0L)
    __mmask8 zeroSeedLanes = _mm512_cmpeq_epi64_mask(state->seedLo, _mm512_set1_epi64(0));
    zeroSeedLanes &= _mm512_cmpeq_epi64_mask(state->seedHi, _mm512_set1_epi64(0));
    if (zeroSeedLanes)
    {
        // s_seed.low = -7046029254386353131L;
        // s_seed.high = 7640891576956012809L;
        state->seedLo = _mm512_mask_set1_epi64(state->seedLo, zeroSeedLanes, -7046029254386353131L);
        state->seedHi = _mm512_mask_set1_epi64(state->seedHi, zeroSeedLanes, 7640891576956012809L);
    }
}

void ltrng_simd_state_mask_set_seed(struct ltrng_simd_State *state, int64_t seed, __mmask8 mask)
{
    long l_seed = seed ^ 0x6A09E667F3BCC909L;

    struct ltrng_Seed s_seed;
    s_seed.low = ltrng_util_mix_stafford_13(l_seed) ^ state->hash.low;
    s_seed.high = ltrng_util_mix_stafford_13(l_seed + -7046029254386353131L) ^ state->hash.high;

    if ((s_seed.low | s_seed.high) == 0L)
    {
        s_seed.low = -7046029254386353131L;
        s_seed.high = 7640891576956012809L;
    }

    state->seedLo = _mm512_mask_set1_epi64(state->seedLo, mask, s_seed.low);
    state->seedHi = _mm512_mask_set1_epi64(state->seedHi, mask, s_seed.high);

    // 1.20-pre3 burns a nextLong() call
    ltrng_simd_state_mask_next_long(state, mask);
}

__m512i ltrng_simd_state_next_long(struct ltrng_simd_State *state)
{
    __m512i l = state->seedLo;
    __m512i m = state->seedHi;

    // return value only
    __m512i n = _mm512_add_epi64(ltrng_simd_util_rotl(_mm512_add_epi64(l, m), 17), l);

    // actual generator
    m = _mm512_xor_epi64(m, l);
    state->seedLo = _mm512_xor_epi64(_mm512_xor_epi64(ltrng_simd_util_rotl(l, 49), m), _mm512_slli_epi64(m, 21));
    state->seedHi = ltrng_simd_util_rotl(m, 28);

    return n;
}

__m512i ltrng_simd_state_mask_next_long(struct ltrng_simd_State *state, __mmask8 mask)
{
    __m512i l = state->seedLo;
    __m512i m = state->seedHi;

    // return value only
    __m512i n = _mm512_maskz_add_epi64(mask, ltrng_simd_util_mask_rotl(_mm512_maskz_add_epi64(mask, l, m), 17, mask), l);

    // actual generator
    m = _mm512_mask_xor_epi64(m, mask, m, l);
    state->seedLo = _mm512_mask_xor_epi64(state->seedLo, mask, _mm512_mask_xor_epi64(state->seedLo, mask, ltrng_simd_util_mask_rotl(l, 49, mask), m), _mm512_mask_slli_epi64(m, mask, m, 21));
    state->seedHi = ltrng_simd_util_mask_rotl(m, 28, mask);

    return n;
}

__m512i ltrng_simd_state_next_int(struct ltrng_simd_State *state)
{
    return _mm512_and_epi64(ltrng_simd_state_next_long(state), _mm512_set1_epi64(0xFFFFFFFF));
}

__m512i ltrng_simd_state_mask_next_int(struct ltrng_simd_State *state, __mmask8 mask)
{
    return _mm512_maskz_and_epi64(mask, ltrng_simd_state_mask_next_long(state, mask), _mm512_set1_epi64(0xFFFFFFFF));
}

__m512i ltrng_simd_state_next_int_range(struct ltrng_simd_State *state, int32_t i)
{
    __m512i l = ltrng_simd_state_next_int(state);
    __m512i m = _mm512_mullo_epi64(l, _mm512_set1_epi64(i));
    __m512i n = _mm512_and_epi64(m, _mm512_set1_epi64(0xFFFFFFFF));

    __mmask8 retryMask = _mm512_cmplt_epi64_mask(n, _mm512_set1_epi64(i));
    if (retryMask)
    {
        int32_t j = ((uint32_t)(~i + 1)) % (uint32_t)i;
        while (retryMask)
        {
            l = _mm512_mask_blend_epi64(retryMask, l, ltrng_simd_state_mask_next_int(state, retryMask));
            m = _mm512_mask_mullo_epi64(m, retryMask, l, _mm512_set1_epi64(i));
            n = _mm512_mask_and_epi64(n, retryMask, m, _mm512_set1_epi64(0xFFFFFFFF));
            retryMask = _mm512_cmplt_epi64_mask(n, _mm512_set1_epi64(j));
        }
    }

    return _mm512_srli_epi64(m, 32);
}
