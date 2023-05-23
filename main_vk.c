
#define LTRNG_VK_VALIDATION

#define STARTING_SEED 157830749991L

#define SHADER_LOCAL_SIZE_X 1024
#define SHADER_GROUP_SIZE_X 4096
#define SHADER_BATCH_SIZE (SHADER_LOCAL_SIZE_X * SHADER_GROUP_SIZE_X)
#define SHADER_SEEDS_PER_INVOKE 1024L

#include "./vulkan/ltrng_vk.h"
#include "./ltrng.c"
#include <stdio.h>
#include <math.h>
#include <time.h>

struct ShaderUniforms
{
    uint64_t seedOffset;
    uint64_t minCount;

    uint64_t hashLo;
    uint64_t hashHi;
};

struct ShaderData
{
    uint64_t count;
    uint64_t seed;
};

int main(int argc, char **argv)
{
    struct ltrng_vk_cfg cfg = {};

    cfg.shaderPath = "shaders/shader.spv";

    cfg.groupCountX = SHADER_GROUP_SIZE_X;

    cfg.groupCountY = 1;
    cfg.groupCountZ = 1;
    cfg.uniformBufferSize = sizeof(struct ShaderUniforms);
    cfg.dataBufferSize = sizeof(struct ShaderData);

    struct ltrng_vk *inst = ltrng_vk_create(&cfg);

    printf("%i\n", sizeof(struct ShaderData) * SHADER_BATCH_SIZE);

    if (!inst)
    {
        printf("An error occured while creating the Vulkan instance.\n");
        return 1;
    }
    printf("Vulkan initalized.\n");

    struct ShaderUniforms *uniforms = ltrng_vk_uniform_buffer_get(inst);

    struct ltrng_State state;
    ltrng_state_init(&state, "minecraft:blocks/melon");
    uniforms->hashHi = state.hash.high;
    uniforms->hashLo = state.hash.low;
    uniforms->minCount = 12;
    uniforms->seedOffset = 0;

    uint32_t biggestCount = 0;
    struct ShaderData *data = ltrng_vk_data_buffer_get(inst);

    uint64_t lastCount = 0;
    uint64_t lastSeed = 0;

    while (true)
    {
        struct timespec start, end;
        clock_gettime(1, &start);
        ltrng_vk_run(inst);
        clock_gettime(1, &end);
        double elapsed = (end.tv_sec - start.tv_sec) +
                         (end.tv_nsec - start.tv_nsec) / 1e9;

        double perSec = (double) (SHADER_BATCH_SIZE * SHADER_SEEDS_PER_INVOKE) / elapsed;

        
        if (data->count != lastCount)
        {
            lastCount = uniforms->minCount = data->count;
            lastSeed = data->seed;
            printf("Found seed with %llu melons. %llu\n", data->count, lastSeed);
        }

        uniforms->seedOffset += (uint64_t) (SHADER_BATCH_SIZE * SHADER_SEEDS_PER_INVOKE);
        
        printf("%llu - %llu Seeds/s Best %llu melons %llu\n", uniforms->seedOffset, (uint64_t) round(perSec), lastCount, lastSeed);

    }
}

#include "./vulkan/ltrng_vk.c"
