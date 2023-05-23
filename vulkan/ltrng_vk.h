#ifndef LTRNG_VK
#define LTRNG_VK

#include <vulkan/vulkan.h>
#include <stdbool.h>

struct ltrng_vk_cfg
{
    const char *shaderPath;
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;

    int uniformBufferSize;
    int dataBufferSize;
};

struct ltrng_vk
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkPipeline computePipeline;
    VkPipelineLayout computePipelineLayout;
    VkShaderModule computeShaderModule;

    VkCommandPool cmdPool;
    VkCommandBuffer cmdBuffer;

    VkDescriptorPool descPool;
    VkDescriptorSet descSet;
    VkDescriptorSetLayout descSetLayout;

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* uniformBufferMapping;

    VkBuffer dataBuffer;
    VkDeviceMemory dataBufferMemory;
    void* dataBufferMapping;

    uint32_t queueFamilyIndex;
    VkQueue queue;

#ifdef LTRNG_VK_VALIDATION
    VkDebugReportCallbackEXT debugReportCallback;
#endif
};

struct ltrng_vk *ltrng_vk_create(struct ltrng_vk_cfg *cfg);
bool ltrng_vk_run(struct ltrng_vk *inst);
bool ltrng_vk_destroy(struct ltrng_vk *inst);

void *ltrng_vk_data_buffer_get(struct ltrng_vk *inst);
void *ltrng_vk_uniform_buffer_get(struct ltrng_vk *inst);

#endif