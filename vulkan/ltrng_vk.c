
#include "./ltrng_vk.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define LTRNG_VK_CHECK_RESULT(arg)                                                       \
    {                                                                                    \
        int LTRNG_VK_CHECK_RESULT_ERR;                                                   \
        if ((LTRNG_VK_CHECK_RESULT_ERR = (arg)) != VK_SUCCESS)                           \
        {                                                                                \
            printf("Got vulkan error %i from '" #arg "'.\n", LTRNG_VK_CHECK_RESULT_ERR); \
            return false;                                                                \
        }                                                                                \
    }

#pragma region Vulkan Initalization

#ifdef LTRNG_VK_VALIDATION
static VKAPI_ATTR VkBool32 VKAPI_CALL ltrng_vk_debug_report_callback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char *pLayerPrefix,
    const char *pMessage,
    void *pUserData)
{
    printf("Debug Report: %s: %s\n", pLayerPrefix, pMessage);

    return VK_FALSE;
}
#endif

bool ltrng_vk_create_instance(struct ltrng_vk *inst)
{
    VkApplicationInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    info.pApplicationName = "LTRNG";
    info.pEngineName = "LTRNG";
    info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &info;

#ifdef LTRNG_VK_VALIDATION
    const char *layerNames[1];
    layerNames[0] = "VK_LAYER_KHRONOS_validation";
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = layerNames;
    const char *extensionNames[1];
    extensionNames[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    createInfo.enabledExtensionCount = 1;
    createInfo.ppEnabledExtensionNames = extensionNames;
#endif

    LTRNG_VK_CHECK_RESULT(vkCreateInstance(&createInfo, NULL, &inst->instance));

#ifdef LTRNG_VK_VALIDATION
    {
        VkDebugReportCallbackCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        createInfo.pfnCallback = &ltrng_vk_debug_report_callback;

        PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(inst->instance, "vkCreateDebugReportCallbackEXT");
        if (vkCreateDebugReportCallbackEXT == NULL)
        {
            printf("Extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME not supported\n.");
            return false;
        }

        LTRNG_VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(inst->instance, &createInfo, NULL, &inst->debugReportCallback));
    }
#endif

    return true;
}

bool ltrng_vk_create_physical_device(struct ltrng_vk *inst)
{
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(inst->instance, &deviceCount, NULL);
    if (deviceCount == 0)
    {
        printf("No vulkan devices avaliable.\n");
        return false;
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(inst->instance, &deviceCount, devices);

    VkPhysicalDevice device = NULL;

    for (int i = 0; i < deviceCount; i++)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            device = devices[i];
            break;
        }
    }

    if (device == NULL)
        device = devices[0];

    inst->physicalDevice = device;

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);
    printf("Using GPU '%s'\n", props.deviceName);

    return true;
}

bool ltrng_vk_create_find_queue_family(struct ltrng_vk *inst)
{
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(inst->physicalDevice, &queueFamilyCount, NULL);

    VkQueueFamilyProperties queueFamilies[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(inst->physicalDevice, &queueFamilyCount, queueFamilies);

    uint32_t i = 0;
    for (; i < queueFamilyCount; ++i)
    {
        VkQueueFamilyProperties props = queueFamilies[i];

        if (props.queueCount > 0 && (props.queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            break;
        }
    }

    if (i == queueFamilyCount)
    {
        printf("Could not find a Vulkan queue which supports compute.\n");
        return false;
    }

    inst->queueFamilyIndex = i;

    return true;
}

bool ltrng_vk_create_logical_device(struct ltrng_vk *inst)
{
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = inst->queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    float queuePriorities = 1.0;
    queueCreateInfo.pQueuePriorities = &queuePriorities;

    VkDeviceCreateInfo deviceCreateInfo = {};

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.shaderInt64 = VK_TRUE;

    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

#ifdef LTRNG_VK_VALIDATION
    const char *layerNames[1];
    layerNames[0] = "VK_LAYER_KHRONOS_validation";
    deviceCreateInfo.enabledLayerCount = 1;
    deviceCreateInfo.ppEnabledLayerNames = layerNames;
#endif

    LTRNG_VK_CHECK_RESULT(vkCreateDevice(inst->physicalDevice, &deviceCreateInfo, NULL, &inst->device)); // create logical device.

    vkGetDeviceQueue(inst->device, inst->queueFamilyIndex, 0, &inst->queue);
    return true;
}

bool ltrng_vk_util_create_buffer(struct ltrng_vk *inst, int size, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags memoryProps, VkDeviceMemory *outMemory, VkBuffer *outBuffer)
{
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = bufferUsage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    LTRNG_VK_CHECK_RESULT(vkCreateBuffer(inst->device, &bufferCreateInfo, NULL, outBuffer));

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(inst->device, *outBuffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(inst->physicalDevice, &memoryProperties);

    allocateInfo.memoryTypeIndex = -1;
    for (int i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if ((memoryRequirements.memoryTypeBits & (1 << i)) &&
            ((memoryProperties.memoryTypes[i].propertyFlags & memoryProps) == memoryProps))
        {
            allocateInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (allocateInfo.memoryTypeIndex == -1)
    {
        printf("Couldn't find acceptable memory type.\n");
        return false;
    }

    LTRNG_VK_CHECK_RESULT(vkAllocateMemory(inst->device, &allocateInfo, NULL, outMemory));
    LTRNG_VK_CHECK_RESULT(vkBindBufferMemory(inst->device, *outBuffer, *outMemory, 0));

    return true;
}

bool ltrng_vk_create_buffers(struct ltrng_vk *inst, struct ltrng_vk_cfg *cfg)
{
    if (!ltrng_vk_util_create_buffer(inst, cfg->dataBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &inst->dataBufferMemory, &inst->dataBuffer))
        return false;
    vkMapMemory(inst->device, inst->dataBufferMemory, 0, VK_WHOLE_SIZE, 0, &inst->dataBufferMapping);

    if (!ltrng_vk_util_create_buffer(inst, cfg->uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &inst->uniformBufferMemory, &inst->uniformBuffer))
        return false;
    vkMapMemory(inst->device, inst->uniformBufferMemory, 0, VK_WHOLE_SIZE, 0, &inst->uniformBufferMapping);

    return true;
}

bool ltrng_vk_create_descriptor_set_layout(struct ltrng_vk *inst)
{
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2];

    descriptorSetLayoutBindings[0].binding = 0;
    descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetLayoutBindings[0].descriptorCount = 1;
    descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    descriptorSetLayoutBindings[1].binding = 1;
    descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorSetLayoutBindings[1].descriptorCount = 1;
    descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.bindingCount = 2;
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
    LTRNG_VK_CHECK_RESULT(vkCreateDescriptorSetLayout(inst->device, &descriptorSetLayoutCreateInfo, NULL, &inst->descSetLayout));

    return true;
}

bool ltrng_vk_create_descriptor_set(struct ltrng_vk *inst, struct ltrng_vk_cfg *cfg)
{
    VkDescriptorPoolSize descriptorPoolSizes[2];
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizes[0].descriptorCount = 1;
    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorPoolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.maxSets = 1;
    descriptorPoolCreateInfo.poolSizeCount = 2;
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;

    LTRNG_VK_CHECK_RESULT(vkCreateDescriptorPool(inst->device, &descriptorPoolCreateInfo, NULL, &inst->descPool));

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = inst->descPool;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    descriptorSetAllocateInfo.pSetLayouts = &inst->descSetLayout;

    LTRNG_VK_CHECK_RESULT(vkAllocateDescriptorSets(inst->device, &descriptorSetAllocateInfo, &inst->descSet));

    VkDescriptorBufferInfo uniformBufferInfo = {};
    uniformBufferInfo.buffer = inst->uniformBuffer;
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = cfg->uniformBufferSize;

    VkWriteDescriptorSet writeDescriptorSets[2] = {{0}};
    writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[0].dstSet = inst->descSet;
    writeDescriptorSets[0].dstBinding = 0;
    writeDescriptorSets[0].descriptorCount = 1;
    writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSets[0].pBufferInfo = &uniformBufferInfo;

    VkDescriptorBufferInfo resultsBufferInfo = {};
    resultsBufferInfo.buffer = inst->dataBuffer;
    resultsBufferInfo.offset = 0;
    resultsBufferInfo.range = cfg->dataBufferSize;

    writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSets[1].dstSet = inst->descSet;
    writeDescriptorSets[1].dstBinding = 1;
    writeDescriptorSets[1].descriptorCount = 1;
    writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSets[1].pBufferInfo = &resultsBufferInfo;

    vkUpdateDescriptorSets(inst->device, 2, writeDescriptorSets, 0, NULL);

    return true;
}

uint32_t *ltrng_vk_util_read_file(uint32_t *length, const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Could not find or open file: %s\n", filename);
    }

    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    long filesizepadded = (long)(ceil(filesize / 4.0)) * 4;

    char *str = malloc(filesizepadded);
    fread(str, filesize, sizeof(char), fp);
    fclose(fp);

    for (int i = filesize; i < filesizepadded; i++)
    {
        str[i] = 0;
    }

    *length = filesizepadded;
    return (uint32_t *)str;
}

bool ltrng_vk_create_compute_pipeline(struct ltrng_vk *inst, struct ltrng_vk_cfg *cfg)
{
    uint32_t filelength;

    uint32_t *code = ltrng_vk_util_read_file(&filelength, cfg->shaderPath);
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pCode = code;
    createInfo.codeSize = filelength;

    LTRNG_VK_CHECK_RESULT(vkCreateShaderModule(inst->device, &createInfo, NULL, &inst->computeShaderModule));
    free(code);

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.module = inst->computeShaderModule;
    shaderStageCreateInfo.pName = "main";

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &inst->descSetLayout;
    LTRNG_VK_CHECK_RESULT(vkCreatePipelineLayout(inst->device, &pipelineLayoutCreateInfo, NULL, &inst->computePipelineLayout));

    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = shaderStageCreateInfo;
    pipelineCreateInfo.layout = inst->computePipelineLayout;

    LTRNG_VK_CHECK_RESULT(vkCreateComputePipelines(
        inst->device, VK_NULL_HANDLE,
        1, &pipelineCreateInfo,
        NULL, &inst->computePipeline));

    return true;
}

bool ltrng_vk_create_cmd_buffer(struct ltrng_vk *inst, struct ltrng_vk_cfg *cfg)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = 0;
    commandPoolCreateInfo.queueFamilyIndex = inst->queueFamilyIndex;
    LTRNG_VK_CHECK_RESULT(vkCreateCommandPool(inst->device, &commandPoolCreateInfo, NULL, &inst->cmdPool));

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = inst->cmdPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
    LTRNG_VK_CHECK_RESULT(vkAllocateCommandBuffers(inst->device, &commandBufferAllocateInfo, &inst->cmdBuffer));

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // beginInfo.flags = VK_COMMAND_BUFFER_USAGE_;
    LTRNG_VK_CHECK_RESULT(vkBeginCommandBuffer(inst->cmdBuffer, &beginInfo));

    vkCmdBindPipeline(inst->cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, inst->computePipeline);
    vkCmdBindDescriptorSets(inst->cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, inst->computePipelineLayout, 0, 1, &inst->descSet, 0, NULL);

    vkCmdDispatch(inst->cmdBuffer, cfg->groupCountX, cfg->groupCountY, cfg->groupCountZ);

    LTRNG_VK_CHECK_RESULT(vkEndCommandBuffer(inst->cmdBuffer));

    return true;
}

#pragma endregion

struct ltrng_vk *ltrng_vk_create(struct ltrng_vk_cfg *cfg)
{
    struct ltrng_vk *inst = malloc(sizeof(struct ltrng_vk));

    if (!ltrng_vk_create_instance(inst))
        return NULL;
    if (!ltrng_vk_create_physical_device(inst))
        return NULL;
    if (!ltrng_vk_create_find_queue_family(inst))
        return NULL;
    if (!ltrng_vk_create_logical_device(inst))
        return NULL;
    if (!ltrng_vk_create_buffers(inst, cfg))
        return NULL;
    if (!ltrng_vk_create_descriptor_set_layout(inst))
        return NULL;
    if (!ltrng_vk_create_descriptor_set(inst, cfg))
        return NULL;
    if (!ltrng_vk_create_compute_pipeline(inst, cfg))
        return NULL;
    if (!ltrng_vk_create_cmd_buffer(inst, cfg))
        return NULL;

    return inst;
}

bool ltrng_vk_run(struct ltrng_vk *inst)
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &inst->cmdBuffer;

    VkFence fence;
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;

    LTRNG_VK_CHECK_RESULT(vkCreateFence(inst->device, &fenceCreateInfo, NULL, &fence));

    LTRNG_VK_CHECK_RESULT(vkQueueSubmit(inst->queue, 1, &submitInfo, fence));

    LTRNG_VK_CHECK_RESULT(vkWaitForFences(inst->device, 1, &fence, VK_TRUE, 100000000000));

    vkDestroyFence(inst->device, fence, NULL);

    return true;
}

bool ltrng_vk_destroy(struct ltrng_vk *inst)
{
#ifdef LTRNG_VK_VALIDATION
    PFN_vkDestroyDebugReportCallbackEXT func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(inst->instance, "vkDestroyDebugReportCallbackEXT");
    if (func == NULL)
    {
        printf("Could not load vkDestroyDebugReportCallbackEXT.\n");
        return false;
    }
    func(inst->instance, inst->debugReportCallback, NULL);
#endif

    vkFreeMemory(inst->device, inst->dataBufferMemory, NULL);
    vkDestroyBuffer(inst->device, inst->dataBuffer, NULL);

    vkFreeMemory(inst->device, inst->uniformBufferMemory, NULL);
    vkDestroyBuffer(inst->device, inst->uniformBuffer, NULL);

    vkDestroyShaderModule(inst->device, inst->computeShaderModule, NULL);
    vkDestroyDescriptorPool(inst->device, inst->descPool, NULL);
    vkDestroyDescriptorSetLayout(inst->device, inst->descSetLayout, NULL);
    vkDestroyPipelineLayout(inst->device, inst->computePipelineLayout, NULL);
    vkDestroyPipeline(inst->device, inst->computePipeline, NULL);
    vkDestroyCommandPool(inst->device, inst->cmdPool, NULL);
    vkDestroyDevice(inst->device, NULL);
    vkDestroyInstance(inst->instance, NULL);

    free(inst);

    return true;
}

void *ltrng_vk_data_buffer_get(struct ltrng_vk *inst)
{
    return inst->dataBufferMapping;
}

void *ltrng_vk_uniform_buffer_get(struct ltrng_vk *inst)
{
    return inst->uniformBufferMapping;
}