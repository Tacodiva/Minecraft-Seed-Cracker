#ifndef LTRNG
#define LTRNG

#include <stdint.h>

struct ltrng_Seed
{
    int64_t low;
    int64_t high;
};

struct ltrng_State
{
    struct ltrng_Seed seed;
    struct ltrng_Seed hash;
};

uint64_t ltrng_util_mix_stafford_13(uint64_t seed);
uint64_t ltrng_util_rotl(uint64_t x, int8_t r);

void ltrng_state_init(struct ltrng_State *state, const char *str);
void ltrng_state_set_seed(struct ltrng_State *state, int64_t seed);

uint64_t ltrng_state_next_long(struct ltrng_State *state);
uint32_t ltrng_state_next_int(struct ltrng_State *state);
int32_t ltrng_state_next_int_range(struct ltrng_State *state, int32_t i);

#endif