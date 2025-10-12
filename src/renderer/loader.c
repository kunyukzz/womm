#include "loader.h"

#if defined(PLATFORM_LINUX)
#    include <dlfcn.h>
#elif defined(PLATFORM_WINDOWS)
#    include <windows.h>
#endif

#define PROC(name, type) ((type)get_proc(name))
#define INST_PROC(inst, name, type) ((type)s_gpa((inst), (name)))
#define DVC_PROC(dvc, name, type) ((type)s_dpa((dvc), (name)))

vk_table_t re = {0};
static void *s_vk_lib = NULL;
static PFN_vkGetInstanceProcAddr s_gpa = VK_NULL_HANDLE;
static PFN_vkGetDeviceProcAddr s_dpa = VK_NULL_HANDLE;

static void *get_proc(const char *name) {
#if defined(PLATFORM_LINUX)
    return dlsym(s_vk_lib, name);
#elif defined(PLATFORM_WINDOWS)
    return (void *)GetProcAddress((HMODULE)s_vk_lib, name);
#else
    return NULL;
#endif
}

VkResult open_library(void) {
#if defined(PLATFORM_LINUX)
    s_vk_lib = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!s_vk_lib) {
        s_vk_lib = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    }
    if (!s_vk_lib) return VK_ERROR_INITIALIZATION_FAILED;

    s_gpa = (PFN_vkGetInstanceProcAddr)dlsym(s_vk_lib, "vkGetInstanceProcAddr");
#elif defined(PLATFORM_WINDOWS)
    s_vk_lib = (void *)LoadLibraryA("vulkan-1.dll");
    if (!s_vk_lib) return VK_ERROR_INITIALIZATION_FAILED;

    s_gpa = (PFN_vkGetInstanceProcAddr)GetProcAddress((HMODULE)s_vk_lib,
                                                      "vkGetInstanceProcAddr");
#endif

    return (s_gpa != VK_NULL_HANDLE) ? VK_SUCCESS
                                     : VK_ERROR_INITIALIZATION_FAILED;
}

bool vk_init(void) {
    if (open_library() != VK_SUCCESS) return false;

    re.vkGetInstanceProcAddr = s_gpa;

    re.vkCreateInstance = PROC("vkCreateInstance", PFN_vkCreateInstance);

    re.vkEnumerateInstanceExtensionProperties =
        PROC("vkEnumerateInstanceExtensionProperties",
             PFN_vkEnumerateInstanceExtensionProperties);

    re.vkEnumerateInstanceLayerProperties =
        PROC("vkEnumerateInstanceLayerProperties",
             PFN_vkEnumerateInstanceLayerProperties);

    re.vkEnumerateInstanceVersion =
        PROC("vkEnumerateInstanceVersion", PFN_vkEnumerateInstanceVersion);

    return (re.vkCreateInstance != VK_NULL_HANDLE);
}

bool vk_load_instance(VkInstance inst) {
    if (!s_gpa) return false;

    // ======================== CORE INSTANCE FUNCTIONS ====================
    re.vkDestroyInstance =
        INST_PROC(inst, "vkDestroyInstance", PFN_vkDestroyInstance);

    re.vkEnumeratePhysicalDevices =
        INST_PROC(inst, "vkEnumeratePhysicalDevices",
                  PFN_vkEnumeratePhysicalDevices);
    re.vkCreateDevice = INST_PROC(inst, "vkCreateDevice", PFN_vkCreateDevice);

    re.vkGetDeviceProcAddr =
        INST_PROC(inst, "vkGetDeviceProcAddr", PFN_vkGetDeviceProcAddr);

    // ================= PHYSICAL DEVICE QUERY FUNCTIONS ====================
    re.vkGetPhysicalDeviceFeatures =
        INST_PROC(inst, "vkGetPhysicalDeviceFeatures",
                  PFN_vkGetPhysicalDeviceFeatures);

    re.vkGetPhysicalDeviceProperties =
        INST_PROC(inst, "vkGetPhysicalDeviceProperties",
                  PFN_vkGetPhysicalDeviceProperties);

    re.vkGetPhysicalDeviceQueueFamilyProperties =
        INST_PROC(inst, "vkGetPhysicalDeviceQueueFamilyProperties",
                  PFN_vkGetPhysicalDeviceQueueFamilyProperties);

    re.vkGetPhysicalDeviceMemoryProperties =
        INST_PROC(inst, "vkGetPhysicalDeviceMemoryProperties",
                  PFN_vkGetPhysicalDeviceMemoryProperties);

    re.vkGetPhysicalDeviceSparseImageFormatProperties =
        INST_PROC(inst, "vkGetPhysicalDeviceSparseImageFormatProperties",
                  PFN_vkGetPhysicalDeviceSparseImageFormatProperties);

    re.vkGetPhysicalDeviceFormatProperties =
        INST_PROC(inst, "vkGetPhysicalDeviceFormatProperties",
                  PFN_vkGetPhysicalDeviceFormatProperties);

    // ======================== SURFACE FUNCTIONS ==========================
#if PLATFORM_LINUX
    re.vkCreateXlibSurfaceKHR =
        INST_PROC(inst, "vkCreateXlibSurfaceKHR", PFN_vkCreateXlibSurfaceKHR);

#elif PLATFORM_WINDOWS
    re.vkCreateWin32SurfaceKHR =
        INST_PROC(inst, "vkCreateWin32SurfaceKHR", PFN_vkCreateWin32SurfaceKHR);
#endif

    // ===================== EXTENSION ENUMERATIONS =========================
    re.vkEnumerateDeviceExtensionProperties =
        INST_PROC(inst, "vkEnumerateDeviceExtensionProperties",
                  PFN_vkEnumerateDeviceExtensionProperties);

    // ===================== COMMON SURFACE FUNCTIONS =======================
    re.vkDestroySurfaceKHR =
        INST_PROC(inst, "vkDestroySurfaceKHR", PFN_vkDestroySurfaceKHR);

    re.vkGetPhysicalDeviceSurfaceSupportKHR =
        INST_PROC(inst, "vkGetPhysicalDeviceSurfaceSupportKHR",
                  PFN_vkGetPhysicalDeviceSurfaceSupportKHR);

    re.vkGetPhysicalDeviceSurfaceCapabilitiesKHR =
        INST_PROC(inst, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
                  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR);

    re.vkGetPhysicalDeviceSurfaceFormatsKHR =
        INST_PROC(inst, "vkGetPhysicalDeviceSurfaceFormatsKHR",
                  PFN_vkGetPhysicalDeviceSurfaceFormatsKHR);

    re.vkGetPhysicalDeviceSurfacePresentModesKHR =
        INST_PROC(inst, "vkGetPhysicalDeviceSurfacePresentModesKHR",
                  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR);

    // ================= DEBUG UTILITIES FUNCTIONS ====================
    re.vkCreateDebugUtilsMessengerEXT =
        INST_PROC(inst, "vkCreateDebugUtilsMessengerEXT",
                  PFN_vkCreateDebugUtilsMessengerEXT);

    re.vkDestroyDebugUtilsMessengerEXT =
        INST_PROC(inst, "vkDestroyDebugUtilsMessengerEXT",
                  PFN_vkDestroyDebugUtilsMessengerEXT);

    s_dpa = re.vkGetDeviceProcAddr;
    return (re.vkCreateDevice && re.vkEnumeratePhysicalDevices);
}

bool vk_load_device(VkDevice dvc) {
    if (!s_dpa) return false;

    // ==================== DEVICE MANAGEMENT ====================
    re.vkDestroyDevice = DVC_PROC(dvc, "vkDestroyDevice", PFN_vkDestroyDevice);

    re.vkDeviceWaitIdle =
        DVC_PROC(dvc, "vkDeviceWaitIdle", PFN_vkDeviceWaitIdle);

    // ==================== QUEUE MANAGEMENT ====================
    re.vkGetDeviceQueue =
        DVC_PROC(dvc, "vkGetDeviceQueue", PFN_vkGetDeviceQueue);

    re.vkQueueSubmit = DVC_PROC(dvc, "vkQueueSubmit", PFN_vkQueueSubmit);

    re.vkQueueWaitIdle = DVC_PROC(dvc, "vkQueueWaitIdle", PFN_vkQueueWaitIdle);

    re.vkQueuePresentKHR =
        DVC_PROC(dvc, "vkQueuePresentKHR", PFN_vkQueuePresentKHR);

    // =============== COMMAND BUFFER MANAGEMENT =================
    re.vkCreateCommandPool =
        DVC_PROC(dvc, "vkCreateCommandPool", PFN_vkCreateCommandPool);

    re.vkDestroyCommandPool =
        DVC_PROC(dvc, "vkDestroyCommandPool", PFN_vkDestroyCommandPool);

    re.vkResetCommandPool =
        DVC_PROC(dvc, "vkResetCommandPool", PFN_vkResetCommandPool);

    re.vkAllocateCommandBuffers =
        DVC_PROC(dvc, "vkAllocateCommandBuffers", PFN_vkAllocateCommandBuffers);

    re.vkFreeCommandBuffers =
        DVC_PROC(dvc, "vkFreeCommandBuffers", PFN_vkFreeCommandBuffers);

    re.vkBeginCommandBuffer =
        DVC_PROC(dvc, "vkBeginCommandBuffer", PFN_vkBeginCommandBuffer);

    re.vkEndCommandBuffer =
        DVC_PROC(dvc, "vkEndCommandBuffer", PFN_vkEndCommandBuffer);

    re.vkResetCommandBuffer =
        DVC_PROC(dvc, "vkResetCommandBuffer", PFN_vkResetCommandBuffer);

    // =========== COMMAND BUFFER RECORDING (Drawing) =============
    re.vkCmdBeginRenderPass =
        DVC_PROC(dvc, "vkCmdBeginRenderPass", PFN_vkCmdBeginRenderPass);

    re.vkCmdNextSubpass =
        DVC_PROC(dvc, "vkCmdNextSubpass", PFN_vkCmdNextSubpass);

    re.vkCmdEndRenderPass =
        DVC_PROC(dvc, "vkCmdEndRenderPass", PFN_vkCmdEndRenderPass);

    re.vkCmdBeginRenderPass2 =
        DVC_PROC(dvc, "vkCmdBeginRenderPass2", PFN_vkCmdBeginRenderPass2);

    re.vkCmdEndRenderPass2 =
        DVC_PROC(dvc, "vkCmdEndRenderPass2", PFN_vkCmdEndRenderPass2);

    re.vkCmdBindPipeline =
        DVC_PROC(dvc, "vkCmdBindPipeline", PFN_vkCmdBindPipeline);

    re.vkCmdBindDescriptorSets =
        DVC_PROC(dvc, "vkCmdBindDescriptorSets", PFN_vkCmdBindDescriptorSets);
    re.vkCmdBindVertexBuffers =
        DVC_PROC(dvc, "vkCmdBindVertexBuffers", PFN_vkCmdBindVertexBuffers);

    re.vkCmdBindIndexBuffer =
        DVC_PROC(dvc, "vkCmdBindIndexBuffer", PFN_vkCmdBindIndexBuffer);

    re.vkCmdDraw = DVC_PROC(dvc, "vkCmdDraw", PFN_vkCmdDraw);

    re.vkCmdDrawIndexed =
        DVC_PROC(dvc, "vkCmdDrawIndexed", PFN_vkCmdDrawIndexed);

    re.vkCmdPushConstants =
        DVC_PROC(dvc, "vkCmdPushConstants", PFN_vkCmdPushConstants);

    // =========== COMMAND BUFFER RECORDING (State) ===============
    re.vkCmdSetViewport =
        DVC_PROC(dvc, "vkCmdSetViewport", PFN_vkCmdSetViewport);

    re.vkCmdSetScissor = DVC_PROC(dvc, "vkCmdSetScissor", PFN_vkCmdSetScissor);

    re.vkCmdSetLineWidth =
        DVC_PROC(dvc, "vkCmdSetLineWidth", PFN_vkCmdSetLineWidth);

    re.vkCmdPipelineBarrier =
        DVC_PROC(dvc, "vkCmdPipelineBarrier", PFN_vkCmdPipelineBarrier);

    // ========= COMMAND BUFFER RECORDING (Clear/Copy) ============
    re.vkCmdClearAttachments =
        DVC_PROC(dvc, "vkCmdClearAttachments", PFN_vkCmdClearAttachments);

    re.vkCmdCopyBuffer = DVC_PROC(dvc, "vkCmdCopyBuffer", PFN_vkCmdCopyBuffer);

    re.vkCmdCopyBufferToImage =
        DVC_PROC(dvc, "vkCmdCopyBufferToImage", PFN_vkCmdCopyBufferToImage);

    re.vkCmdCopyImage = DVC_PROC(dvc, "vkCmdCopyImage", PFN_vkCmdCopyImage);

    re.vkCmdBlitImage = DVC_PROC(dvc, "vkCmdBlitImage", PFN_vkCmdBlitImage);

    re.vkCmdFillBuffer = DVC_PROC(dvc, "vkCmdFillBuffer", PFN_vkCmdFillBuffer);

    // ==================== MEMORY MANAGEMENT ====================
    re.vkAllocateMemory =
        DVC_PROC(dvc, "vkAllocateMemory", PFN_vkAllocateMemory);

    re.vkFreeMemory = DVC_PROC(dvc, "vkFreeMemory", PFN_vkFreeMemory);

    re.vkMapMemory = DVC_PROC(dvc, "vkMapMemory", PFN_vkMapMemory);

    re.vkUnmapMemory = DVC_PROC(dvc, "vkUnmapMemory", PFN_vkUnmapMemory);

    re.vkGetBufferMemoryRequirements =
        DVC_PROC(dvc, "vkGetBufferMemoryRequirements",
                 PFN_vkGetBufferMemoryRequirements);

    re.vkGetImageMemoryRequirements =
        DVC_PROC(dvc, "vkGetImageMemoryRequirements",
                 PFN_vkGetImageMemoryRequirements);

    re.vkBindBufferMemory =
        DVC_PROC(dvc, "vkBindBufferMemory", PFN_vkBindBufferMemory);

    re.vkBindImageMemory =
        DVC_PROC(dvc, "vkBindImageMemory", PFN_vkBindImageMemory);

    re.vkBindBufferMemory2 =
        DVC_PROC(dvc, "vkBindBufferMemory2", PFN_vkBindBufferMemory2);

    re.vkBindImageMemory2 =
        DVC_PROC(dvc, "vkBindImageMemory2", PFN_vkBindImageMemory2);

    // ============= RESOURCE CREATION/DESTRUCTION ==============
    re.vkCreateBuffer = DVC_PROC(dvc, "vkCreateBuffer", PFN_vkCreateBuffer);

    re.vkDestroyBuffer = DVC_PROC(dvc, "vkDestroyBuffer", PFN_vkDestroyBuffer);

    re.vkCreateImage = DVC_PROC(dvc, "vkCreateImage", PFN_vkCreateImage);

    re.vkDestroyImage = DVC_PROC(dvc, "vkDestroyImage", PFN_vkDestroyImage);

    re.vkCreateImageView =
        DVC_PROC(dvc, "vkCreateImageView", PFN_vkCreateImageView);

    re.vkDestroyImageView =
        DVC_PROC(dvc, "vkDestroyImageView", PFN_vkDestroyImageView);

    re.vkCreateSampler = DVC_PROC(dvc, "vkCreateSampler", PFN_vkCreateSampler);

    re.vkDestroySampler =
        DVC_PROC(dvc, "vkDestroySampler", PFN_vkDestroySampler);

    // ================= DESCRIPTOR MANAGEMENT ====================
    re.vkCreateDescriptorSetLayout =
        DVC_PROC(dvc, "vkCreateDescriptorSetLayout",
                 PFN_vkCreateDescriptorSetLayout);

    re.vkDestroyDescriptorSetLayout =
        DVC_PROC(dvc, "vkDestroyDescriptorSetLayout",
                 PFN_vkDestroyDescriptorSetLayout);

    re.vkCreateDescriptorPool =
        DVC_PROC(dvc, "vkCreateDescriptorPool", PFN_vkCreateDescriptorPool);

    re.vkDestroyDescriptorPool =
        DVC_PROC(dvc, "vkDestroyDescriptorPool", PFN_vkDestroyDescriptorPool);

    re.vkAllocateDescriptorSets =
        DVC_PROC(dvc, "vkAllocateDescriptorSets", PFN_vkAllocateDescriptorSets);

    re.vkFreeDescriptorSets =
        DVC_PROC(dvc, "vkFreeDescriptorSets", PFN_vkFreeDescriptorSets);

    re.vkUpdateDescriptorSets =
        DVC_PROC(dvc, "vkUpdateDescriptorSets", PFN_vkUpdateDescriptorSets);

    // ================== PIPELINE MANAGEMENT =====================
    re.vkCreatePipelineLayout =
        DVC_PROC(dvc, "vkCreatePipelineLayout", PFN_vkCreatePipelineLayout);

    re.vkDestroyPipelineLayout =
        DVC_PROC(dvc, "vkDestroyPipelineLayout", PFN_vkDestroyPipelineLayout);

    re.vkCreateShaderModule =
        DVC_PROC(dvc, "vkCreateShaderModule", PFN_vkCreateShaderModule);

    re.vkDestroyShaderModule =
        DVC_PROC(dvc, "vkDestroyShaderModule", PFN_vkDestroyShaderModule);

    re.vkCreateGraphicsPipelines = DVC_PROC(dvc, "vkCreateGraphicsPipelines",
                                            PFN_vkCreateGraphicsPipelines);

    re.vkDestroyPipeline =
        DVC_PROC(dvc, "vkDestroyPipeline", PFN_vkDestroyPipeline);

    // ================= RENDERPASS MANAGEMENT ====================
    re.vkCreateRenderPass =
        DVC_PROC(dvc, "vkCreateRenderPass", PFN_vkCreateRenderPass);

    re.vkDestroyRenderPass =
        DVC_PROC(dvc, "vkDestroyRenderPass", PFN_vkDestroyRenderPass);

    re.vkCreateFramebuffer =
        DVC_PROC(dvc, "vkCreateFramebuffer", PFN_vkCreateFramebuffer);

    re.vkDestroyFramebuffer =
        DVC_PROC(dvc, "vkDestroyFramebuffer", PFN_vkDestroyFramebuffer);

    // ==================== SYNCHRONIZATION ====================
    re.vkCreateSemaphore =
        DVC_PROC(dvc, "vkCreateSemaphore", PFN_vkCreateSemaphore);

    re.vkDestroySemaphore =
        DVC_PROC(dvc, "vkDestroySemaphore", PFN_vkDestroySemaphore);

    re.vkCreateFence = DVC_PROC(dvc, "vkCreateFence", PFN_vkCreateFence);

    re.vkWaitForFences = DVC_PROC(dvc, "vkWaitForFences", PFN_vkWaitForFences);

    re.vkResetFences = DVC_PROC(dvc, "vkResetFences", PFN_vkResetFences);

    re.vkDestroyFence = DVC_PROC(dvc, "vkDestroyFence", PFN_vkDestroyFence);

    // ================= SWAPCHAIN MANAGEMENT ====================
    re.vkCreateSwapchainKHR =
        DVC_PROC(dvc, "vkCreateSwapchainKHR", PFN_vkCreateSwapchainKHR);

    re.vkDestroySwapchainKHR =
        DVC_PROC(dvc, "vkDestroySwapchainKHR", PFN_vkDestroySwapchainKHR);

    re.vkGetSwapchainImagesKHR =
        DVC_PROC(dvc, "vkGetSwapchainImagesKHR", PFN_vkGetSwapchainImagesKHR);

    re.vkAcquireNextImageKHR =
        DVC_PROC(dvc, "vkAcquireNextImageKHR", PFN_vkAcquireNextImageKHR);

// ==================== DEBUG UTILITIES ====================
#ifdef JNK_DEBUG
    re.vkSetDebugUtilsObjectNameEXT =
        DVC_PROC(dvc, "vkSetDebugUtilsObjectNameEXT",
                 PFN_vkSetDebugUtilsObjectNameEXT);

    re.vkSetDebugUtilsObjectTagEXT =
        DVC_PROC(dvc, "vkSetDebugUtilsObjectTagEXT",
                 PFN_vkSetDebugUtilsObjectTagEXT);
#endif
    return (re.vkDeviceWaitIdle != VK_NULL_HANDLE);
}

void vk_kill(void) {
    if (s_vk_lib) {
        dlclose(s_vk_lib);
        s_vk_lib = NULL;
    }
}
