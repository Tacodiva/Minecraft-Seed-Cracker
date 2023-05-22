#include "./ltrng.h"

#include <openssl/evp.h>
#include <string.h>

uint64_t ltrng_util_mix_stafford_13(uint64_t seed)
{
    seed = (seed ^ seed >> 30) * -4658895280553007687L;
    seed = (seed ^ seed >> 27) * -7723592293110705685L;
    return seed ^ seed >> 31;
}

uint64_t ltrng_util_rotl(uint64_t x, int8_t r)
{
    return (x << r) | (x >> (64 - r));
}

void ltrng_state_init(struct ltrng_State *state, const char *str)
{
    EVP_MD_CTX *md5_context;
    /* MD5_Init */
    md5_context = EVP_MD_CTX_new();
    EVP_DigestInit_ex(md5_context, EVP_md5(), NULL);
    /* MD5_Update */
    EVP_DigestUpdate(md5_context, str, strlen(str));
    /* MD5_Final */
    EVP_DigestFinal_ex(md5_context, (void *)&state->hash, NULL);

    state->hash.high = __bswap_64(state->hash.high);
    state->hash.low = __bswap_64(state->hash.low);

    EVP_MD_CTX_free(md5_context);
}

void ltrng_state_set_seed(struct ltrng_State *state, int64_t seed)
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

    state->seed = s_seed;

    // 1.20-pre3 burns a nextLong() call
    ltrng_state_next_long(state);
}

uint64_t ltrng_state_next_long(struct ltrng_State *state)
{
    uint64_t l = state->seed.low;
    uint64_t m = state->seed.high;
    // return value only
    uint64_t n = ltrng_util_rotl(l + m, 17) + l;

    // actual generator
    m ^= l;
    state->seed.low = (ltrng_util_rotl(l, 49) ^ m) ^ (m << 21);
    state->seed.high = ltrng_util_rotl(m, 28);

    return n;
}

uint32_t ltrng_state_next_int(struct ltrng_State *state)
{
    return (uint32_t)ltrng_state_next_long(state);
}

int32_t ltrng_state_next_int_range(struct ltrng_State *state, int32_t i)
{
    uint64_t l = ltrng_state_next_int(state);
    uint64_t m = l * (uint64_t)i;
    uint64_t n = m & 0xFFFFFFFF;

    if (n < (uint64_t)i)
    {
        for (int32_t j = ((uint32_t)(~i + 1)) % (uint32_t)i; n < j; n = m & 0xFFFFFFFF)
        {
            l = ltrng_state_next_int(state);
            m = l * i;
        }
    }
    return m >> 32;
}
