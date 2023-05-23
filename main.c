#include <stdlib.h>
#include <pthread.h>

#include "./ltrng.c"

#define THREAD_COUNT 1
#define STARTING_SEED 0L

int test_loot(struct ltrng_State *state)
{
    // Rewrite this for each loot table. Be careful since loot tables might do extra
    // calls besides what you target,
    // which this method must do as well. That is, this method should imitate a full
    // call to the loot table.
    // You must look in the code to determine exactly the nature of the needed test,
    // or experiment

    // determines melon slice count
    int32_t val = ltrng_state_next_int_range(state, 5);

    // melons have something extra in their loot table which does a call after the
    // number of slices is chosen
    // the next line accounts for that
    ltrng_state_next_long(state);

    return (val == 4);
}

int max = 0;
pthread_mutex_t max_mtx = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *data)
{
    uint32_t threadID = (uint32_t)(uint64_t)data;

    struct ltrng_State state;
    int thread_max = 0;

    ltrng_state_init(&state, "minecraft:blocks/melon");

    for (int64_t seed = threadID + STARTING_SEED;; seed += THREAD_COUNT)
    {
        ltrng_state_set_seed(&state, seed);

        int count = 0;
        while (test_loot(&state))
        {
            ++count;
        }

        if (count > thread_max)
        {
            pthread_mutex_lock(&max_mtx);
            thread_max = max;
            if (count > thread_max)
            {
                max = count;
                thread_max = count;
                printf("Found %i Melons Seed = %lld\n", count, seed);
            }
            pthread_mutex_unlock(&max_mtx);
        }
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