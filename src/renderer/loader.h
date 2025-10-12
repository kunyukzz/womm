#ifndef VULKAN_LOADER_H
#define VULKAN_LOADER_H

#include "core/define.h"

#if defined(VULKAN_H_) && !defined(VK_NO_PROTOTYPES)
#    error "Define VK_NO_PROTOTYPES before including vulkan.h"
#endif

#ifndef VK_NO_PROTOTYPES
#    define VK_NO_PROTOTYPES
#endif

#if PLATFORM_LINUX
#    define VK_USE_PLATFORM_XLIB_KHR 1
#elif PLATFORM_WINDOWS
#    define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#if defined(PLATFORM_LINUX)
typedef struct _XDisplay Display;
typedef unsigned long Window;
typedef unsigned long VisualID;
#    include <vulkan/vulkan_xlib.h>
#elif defined(PLATFORM_WINDOWS)
typedef unsigned long DWORD;
typedef const wchar_t *LPCWSTR;
typedef void *HANDLE;
typedef struct HINSTANCE__ *HINSTANCE;
typedef struct HWND__ *HWNDR;
typedef struct HMONITOR__ *HMONITOR;
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;
#    include <vulkan/vulkan_win32.h>
#endif

typedef struct vk_table_t {
    // ========================== GLOBAL FUNCTIONS =========================
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    PFN_vkCreateInstance vkCreateInstance;

    PFN_vkEnumerateInstanceExtensionProperties
        vkEnumerateInstanceExtensionProperties;
    PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
    PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;

    // ======================== CORE INSTANCE FUNCTIONS ====================
    PFN_vkDestroyInstance vkDestroyInstance;
    PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
    PFN_vkCreateDevice vkCreateDevice;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

    // ================= PHYSICAL DEVICE QUERY FUNCTIONS ====================
    PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties
        vkGetPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties
        vkGetPhysicalDeviceSparseImageFormatProperties;
    PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;

    // ======================== SURFACE FUNCTIONS ==========================
#if PLATFORM_LINUX
    PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
#elif PLATFORM_WINDOWS
    PFN vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#endif

    // ===================== EXTENSION ENUMERATIONS =========================
    PFN_vkEnumerateDeviceExtensionProperties
        vkEnumerateDeviceExtensionProperties;

    // ===================== COMMON SURFACE FUNCTIONS =======================
    PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR
        vkGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR
        vkGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
        vkGetPhysicalDeviceSurfacePresentModesKHR;

    // ================= DEBUG UTILITIES FUNCTIONS ====================
#ifdef DEBUG
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
#endif

    // ==================== DEVICE MANAGEMENT ====================
    PFN_vkDestroyDevice vkDestroyDevice;
    PFN_vkDeviceWaitIdle vkDeviceWaitIdle;

    // ==================== QUEUE MANAGEMENT =====================
    PFN_vkGetDeviceQueue vkGetDeviceQueue;
    PFN_vkQueueSubmit vkQueueSubmit;
    PFN_vkQueueWaitIdle vkQueueWaitIdle;
    PFN_vkQueuePresentKHR vkQueuePresentKHR;

    // =============== COMMAND BUFFER MANAGEMENT =================
    PFN_vkCreateCommandPool vkCreateCommandPool;
    PFN_vkDestroyCommandPool vkDestroyCommandPool;
    PFN_vkResetCommandPool vkResetCommandPool;
    PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
    PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
    PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
    PFN_vkEndCommandBuffer vkEndCommandBuffer;
    PFN_vkResetCommandBuffer vkResetCommandBuffer;

    // =========== COMMAND BUFFER RECORDING (Drawing) =============
    PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
    PFN_vkCmdNextSubpass vkCmdNextSubpass;
    PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
    PFN_vkCmdBeginRenderPass2 vkCmdBeginRenderPass2;
    PFN_vkCmdEndRenderPass2 vkCmdEndRenderPass2;
    PFN_vkCmdBindPipeline vkCmdBindPipeline;
    PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
    PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
    PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
    PFN_vkCmdDraw vkCmdDraw;
    PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
    PFN_vkCmdPushConstants vkCmdPushConstants;

    // =========== COMMAND BUFFER RECORDING (State) ===============
    PFN_vkCmdSetViewport vkCmdSetViewport;
    PFN_vkCmdSetScissor vkCmdSetScissor;
    PFN_vkCmdSetLineWidth vkCmdSetLineWidth;
    PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;

    // ========= COMMAND BUFFER RECORDING (Clear/Copy) ============
    PFN_vkCmdClearAttachments vkCmdClearAttachments;
    PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
    PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
    PFN_vkCmdCopyImage vkCmdCopyImage;
    PFN_vkCmdBlitImage vkCmdBlitImage;
    PFN_vkCmdFillBuffer vkCmdFillBuffer;

    // ==================== MEMORY MANAGEMENT ====================
    PFN_vkAllocateMemory vkAllocateMemory;
    PFN_vkFreeMemory vkFreeMemory;
    PFN_vkMapMemory vkMapMemory;
    PFN_vkUnmapMemory vkUnmapMemory;
    PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
    PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
    PFN_vkBindBufferMemory vkBindBufferMemory;
    PFN_vkBindImageMemory vkBindImageMemory;
    PFN_vkBindBufferMemory2 vkBindBufferMemory2;
    PFN_vkBindImageMemory2 vkBindImageMemory2;

    // ============= RESOURCE CREATION/DESTRUCTION ==============
    PFN_vkCreateBuffer vkCreateBuffer;
    PFN_vkDestroyBuffer vkDestroyBuffer;
    PFN_vkCreateImage vkCreateImage;
    PFN_vkDestroyImage vkDestroyImage;
    PFN_vkCreateImageView vkCreateImageView;
    PFN_vkDestroyImageView vkDestroyImageView;
    PFN_vkCreateSampler vkCreateSampler;
    PFN_vkDestroySampler vkDestroySampler;

    // ================= DESCRIPTOR MANAGEMENT ====================
    PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
    PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
    PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
    PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
    PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
    PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
    PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;

    // ================== PIPELINE MANAGEMENT =====================
    PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
    PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
    PFN_vkCreateShaderModule vkCreateShaderModule;
    PFN_vkDestroyShaderModule vkDestroyShaderModule;
    PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
    PFN_vkDestroyPipeline vkDestroyPipeline;

    // ================= RENDERPASS MANAGEMENT ====================
    PFN_vkCreateRenderPass vkCreateRenderPass;
    PFN_vkDestroyRenderPass vkDestroyRenderPass;
    PFN_vkCreateFramebuffer vkCreateFramebuffer;
    PFN_vkDestroyFramebuffer vkDestroyFramebuffer;

    // ==================== SYNCHRONIZATION ====================
    PFN_vkCreateSemaphore vkCreateSemaphore;
    PFN_vkDestroySemaphore vkDestroySemaphore;
    PFN_vkCreateFence vkCreateFence;
    PFN_vkWaitForFences vkWaitForFences;
    PFN_vkResetFences vkResetFences;
    PFN_vkDestroyFence vkDestroyFence;

    // ================= SWAPCHAIN MANAGEMENT ====================
    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;

// ==================== DEBUG UTILITIES ====================
#ifdef DEBUG
    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
    PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTagEXT;
#endif
} vk_table_t;

extern vk_table_t re;

bool vk_init(void);
bool vk_load_instance(VkInstance inst);
bool vk_load_device(VkDevice dvc);
void vk_kill(void);

#endif // VULKAN_LOADER_H
