#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

#include "./ltrng.c"
#include "./ltrng_simd.c"

#define THREAD_COUNT 8
#define STARTING_SEED 0L

__mmask8 test_loot(struct ltrng_simd_State *state)
{

    __m512i val = ltrng_simd_state_next_int_range(state, 5);

    // melons have something extra in their loot table which does a call after the
    // number of slices is chosen
    // the next line accounts for that
    ltrng_simd_state_next_long(state);

    return _mm512_cmpeq_epi64_mask(val, _mm512_set1_epi64(4));
}

int max = 0;
pthread_mutex_t max_mtx = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *data)
{
    uint32_t threadID = (uint32_t)(uint64_t)data;

    struct ltrng_simd_State state;
    ltrng_simd_state_init(&state, "minecraft:blocks/melon");

    int thread_max = 0;

    int64_t stackSeeds[8];
    int64_t next_seed = STARTING_SEED + threadID;
    for (int i = 0; i < 8; i++)
    {
        stackSeeds[i] = next_seed;
        next_seed += THREAD_COUNT;
    }

    __m512i seeds = _mm512_load_epi64(stackSeeds);

    while (true)
    {
        ltrng_simd_state_set_seeds(&state, seeds);

        int count = -1;
        __mmask8 lastValidSeeds = 0;
        __mmask8 validSeeds = 0xFF;
        while (validSeeds)
        {
            ++count;
            lastValidSeeds = validSeeds;
            validSeeds &= test_loot(&state);
        }

        if (count > thread_max)
        {
            pthread_mutex_lock(&max_mtx);
            thread_max = max;
            if (count > thread_max)
            {
                max = count;
                thread_max = count;

                int seedLane;
                for (seedLane = 0; seedLane < 8; seedLane++)
                    if (lastValidSeeds & (1 << seedLane))
                        break;

                _mm512_storeu_si512(stackSeeds, seeds);
                int64_t seed = stackSeeds[seedLane];

                printf("Found %i Melons Seed = %lld\n", count, seed);
                // if (max == 15)
                //     kill(getpid(), SIGKILL);
            }
            pthread_mutex_unlock(&max_mtx);
        }

        seeds = _mm512_add_epi64(seeds, _mm512_set1_epi64(THREAD_COUNT * 8));
    }
}

int main(int argc, char **argv)
{
    pthread_t thread;
    for (uint32_t i = 0; i < THREAD_COUNT - 1; i++)
    {
        pthread_create(&thread, NULL, worker, (void *)(uint64_t)i);
    }
    worker((void *)(uint64_t)THREAD_COUNT - 1);
    return 0;
}