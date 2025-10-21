#include "backend.h"
#include "core/memory.h"
#include "core/paths.h"
#include "core/binary_loader.h"

#include <string.h> // strcmp
#include <stdio.h>

/************************************
 * CORE
 ************************************/
#if DEBUG
static const char *const req_validation_layers[] = {
    "VK_LAYER_KHRONOS_validation",
};
#endif

static const char *const req_device_ext[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

static const char *const req_instance_ext[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(PLATFORM_LINUX)
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif defined(PLATFORM_WINDOWS)
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
#    error "Other platform not supported"
#endif

#if DEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};

static uint32_t find_mem_type(vk_core_t *core, uint32_t type_filter,
                              VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mem_props;
    re.vkGetPhysicalDeviceMemoryProperties(core->gpu, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if ((type_filter & (1 << i)) &&
            (mem_props.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }

    LOG_FATAL("Failed to find suitable memory type");
    return INVALID_32;
}

#if DEBUG

static VKAPI_ATTR VkBool32 VKAPI_CALL dbg_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
    VkDebugUtilsMessageTypeFlagsEXT msg_type,
    const VkDebugUtilsMessengerCallbackDataEXT *_callback, void *user_data) {
    (void)msg_type;
    (void)user_data;

    switch (msg_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            LOG_ERROR(_callback->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_ERROR(_callback->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            // LOG_INFO(_callback->pMessage);
            break;
        default: break;
    }
    return VK_FALSE;
}

static bool init_debugger(vk_core_t *core) {
    uint32_t log = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.messageSeverity = log;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    debug_info.pfnUserCallback = dbg_callback;

    CHECK_VK(re.vkCreateDebugUtilsMessengerEXT(core->instance, &debug_info,
                                               core->alloc, &core->util_dbg));

    if (!vk_load_instance(core->instance)) {
        LOG_FATAL("failed to load debugger vulkan");
        return false;
    }

    LOG_DEBUG("vulkan debugger initialize");
    return true;
}

static bool chk_validation_support(vk_core_t *core) {
    (void)core;
    uint32_t count = 0;
    re.vkEnumerateInstanceLayerProperties(&count, NULL);
    VkLayerProperties *avail =
        (VkLayerProperties *)WALLOC(sizeof(VkLayerProperties) * count,
                                    MEM_RENDER);

    re.vkEnumerateInstanceLayerProperties(&count, avail);

    for (uint32_t i = 0; i < ARRAY_SIZE(req_validation_layers); ++i) {
        bool found = false;
        for (uint32_t j = 0; j < count; ++j) {
            if (strcmp(req_validation_layers[i], avail[j].layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            WFREE(avail, sizeof(VkLayerProperties) * count, MEM_RENDER);
            return false;
        }
    }

    WFREE(avail, sizeof(VkLayerProperties) * count, MEM_RENDER);
    return true;
}

#endif

void get_req_ext(const char *const **out_list, uint32_t *ext_count) {
    if (out_list) *out_list = req_instance_ext;
    if (ext_count) *ext_count = ARRAY_SIZE(req_instance_ext);
}

static bool chk_dvc_support(VkPhysicalDevice gpu) {
    uint32_t count = 0;

    re.vkEnumerateDeviceExtensionProperties(gpu, NULL, &count, NULL);
    VkExtensionProperties *avail =
        (VkExtensionProperties *)WALLOC(sizeof(VkExtensionProperties) * count,
                                        MEM_RENDER);

    re.vkEnumerateDeviceExtensionProperties(gpu, NULL, &count, avail);

    VkPhysicalDeviceProperties dvc_prop;
    re.vkGetPhysicalDeviceProperties(gpu, &dvc_prop);

    for (uint32_t i = 0; i < ARRAY_SIZE(req_device_ext); ++i) {
        bool found = false;

        for (uint32_t j = 0; j < count; ++j) {
            if (strcmp(req_device_ext[i], avail[j].extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            WFREE(avail, sizeof(VkExtensionProperties) * count, MEM_RENDER);
            return false;
        }
    }

    WFREE(avail, sizeof(VkExtensionProperties) * count, MEM_RENDER);
    return true;
}

static int32_t score_device(VkPhysicalDevice gpu) {
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceMemoryProperties mems;
    re.vkGetPhysicalDeviceProperties(gpu, &props);
    re.vkGetPhysicalDeviceMemoryProperties(gpu, &mems);

    int32_t score = 0;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        score += 500;

    score += props.limits.maxImageDimension2D;

    for (uint32_t i = 0; i < mems.memoryHeapCount; ++i) {
        if (mems.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            score += (mems.memoryHeaps[i].size / (128 * 1024 * 1024));
    }
    return score;
}

static bool pick_physical_device(vk_core_t *core) {
    uint32_t count = 0;
    re.vkEnumeratePhysicalDevices(core->instance, &count, NULL);
    if (count == 0) return false;

    VkPhysicalDevice *gpus =
        WALLOC(sizeof(VkPhysicalDevice) * count, MEM_RENDER);
    re.vkEnumeratePhysicalDevices(core->instance, &count, gpus);

    VkPhysicalDevice best = VK_NULL_HANDLE;
    int32_t best_score = -1;

    for (uint32_t i = 0; i < count; ++i) {
        if (!chk_dvc_support(gpus[i])) continue;

        int32_t score = score_device(gpus[i]);
        if (score > best_score) {
            best_score = score;
            best = gpus[i];
        }
    }

    WFREE(gpus, sizeof(VkPhysicalDevice) * count, MEM_RENDER);
    if (best == VK_NULL_HANDLE) return false;

    core->gpu = best;
    re.vkGetPhysicalDeviceProperties(core->gpu, &core->properties);
    re.vkGetPhysicalDeviceFeatures(core->gpu, &core->features);
    re.vkGetPhysicalDeviceMemoryProperties(core->gpu, &core->mem_prop);

    LOG_DEBUG("Selected GPU: %s", core->properties.deviceName);
    return true;
}

static VkFormat find_supported_format(vk_core_t *core,
                                      const VkFormat *candidates,
                                      uint32_t candidate_count,
                                      VkImageTiling tiling,
                                      VkFormatFeatureFlags features) {
    for (uint32_t i = 0; i < candidate_count; ++i) {
        VkFormatProperties props;
        re.vkGetPhysicalDeviceFormatProperties(core->gpu, candidates[i],
                                               &props);

        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features)
            return candidates[i];
        if (tiling == VK_IMAGE_TILING_OPTIMAL &&
            (props.optimalTilingFeatures & features) == features)
            return candidates[i];
    }
    return VK_FORMAT_UNDEFINED;
}

static bool pick_queue_families(vk_core_t *core) {
    uint32_t count;
    re.vkGetPhysicalDeviceQueueFamilyProperties(core->gpu, &count, NULL);
    VkQueueFamilyProperties *families =
        WALLOC(sizeof(VkQueueFamilyProperties) * count, MEM_RENDER);
    re.vkGetPhysicalDeviceQueueFamilyProperties(core->gpu, &count, families);

    core->graphic_idx = UINT32_MAX;
    core->transfer_idx = UINT32_MAX;

    for (uint32_t i = 0; i < count; ++i) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            core->graphic_idx = i;

        if ((families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            !(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            core->transfer_idx = i;
    }

    if (core->transfer_idx == UINT32_MAX)
        core->transfer_idx = core->graphic_idx;

    WFREE(families, sizeof(VkQueueFamilyProperties) * count, MEM_RENDER);
    return (core->graphic_idx != UINT32_MAX);
}

static bool create_logical_device(vk_core_t *core) {
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queues[2] = {};
    uint32_t unique_count = (core->graphic_idx != core->transfer_idx) ? 2 : 1;

    queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queues[0].queueFamilyIndex = core->graphic_idx;
    queues[0].queueCount = 1;
    queues[0].pQueuePriorities = &priority;

    if (unique_count == 2) {
        queues[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queues[1].queueFamilyIndex = core->transfer_idx;
        queues[1].queueCount = 1;
        queues[1].pQueuePriorities = &priority;
    }

    VkPhysicalDeviceFeatures enable_feats = {};
    enable_feats.fillModeNonSolid = VK_TRUE;
    enable_feats.multiDrawIndirect = VK_TRUE;
    enable_feats.depthClamp = VK_TRUE;
    enable_feats.wideLines = VK_TRUE;

    VkDeviceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = unique_count;
    info.pQueueCreateInfos = queues;
    info.enabledExtensionCount = ARRAY_SIZE(req_device_ext);
    info.ppEnabledExtensionNames = req_device_ext;
    info.pEnabledFeatures = &enable_feats;

    CHECK_VK(
        re.vkCreateDevice(core->gpu, &info, core->alloc, &core->logic_dvc));

    if (!vk_load_device(core->logic_dvc)) return false;

    re.vkGetDeviceQueue(core->logic_dvc, core->graphic_idx, 0,
                        &core->graphic_queue);
    re.vkGetDeviceQueue(core->logic_dvc, core->transfer_idx, 0,
                        &core->transfer_queue);
    re.vkGetDeviceQueue(core->logic_dvc, core->present_idx, 0,
                        &core->present_queue);

    LOG_DEBUG("vulkan device initialize");
    return true;
}

static bool create_command_pool(vk_core_t *core) {
    VkCommandPoolCreateInfo pool = {};
    pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool.queueFamilyIndex = core->graphic_idx;
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    CHECK_VK(re.vkCreateCommandPool(core->logic_dvc, &pool, core->alloc,
                                    &core->gfx_pool));

    LOG_DEBUG("vulkan command pool initialize");
    return true;
}

static bool init_instance(vk_core_t *core) {
    if (!vk_init()) {
        LOG_FATAL("Failed to load global vulkan table");
        return false;
    }
    core->alloc = NULL;

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_API_VERSION_1_0;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pApplicationName = "No Engine";

    const char *const *inst_exts;
    uint32_t inst_ext_count;
    get_req_ext(&inst_exts, &inst_ext_count);

    uint32_t layer_count = 0;
    const char *const *layer_names = NULL;

#if DEBUG
    if (chk_validation_support(core)) {
        layer_count = ARRAY_SIZE(req_validation_layers);
        layer_names = req_validation_layers;
    }
#endif

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledExtensionCount = inst_ext_count;
    inst_info.ppEnabledExtensionNames = inst_exts;
    inst_info.enabledLayerCount = layer_count;
    inst_info.ppEnabledLayerNames = layer_names;

    CHECK_VK(re.vkCreateInstance(&inst_info, core->alloc, &core->instance));

    if (!vk_load_instance(core->instance)) {
        LOG_FATAL("Failed to load instance vulkan table");
        return false;
    }

    return true;
}

static bool memory_init(vk_core_t *core) {
    core->memories.total_allocated = 0;
    core->memories.dvc_local_used = 0;
    core->memories.host_visible_used = 0;

    core->memories.budget = 1 * 1024 * 1024 * 1024ULL;
    return true;
}

bool core_init(vk_core_t *core) {
    if (!init_instance(core)) return false;

#if DEBUG
    if (!init_debugger(core)) return false;
#endif

    if (!pick_physical_device(core)) return false;
    if (!pick_queue_families(core)) return false;

    VkFormat depth_candidates[] = {VK_FORMAT_D32_SFLOAT,
                                   VK_FORMAT_D32_SFLOAT_S8_UINT,
                                   VK_FORMAT_D24_UNORM_S8_UINT};

    core->default_depth_format =
        find_supported_format(core, depth_candidates,
                              ARRAY_SIZE(depth_candidates),
                              VK_IMAGE_TILING_OPTIMAL,
                              VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    if (core->default_depth_format == VK_FORMAT_UNDEFINED) {
        LOG_FATAL("Failed to find a supported depth format");
        return false;
    }

    if (!create_logical_device(core)) return false;
    if (!create_command_pool(core)) return false;
    if (!memory_init(core)) return false;

    LOG_DEBUG("vulkan core initialize");
    return true;
}

void core_kill(vk_core_t *core) {
    re.vkDestroyCommandPool(core->logic_dvc, core->gfx_pool, core->alloc);
    core->gfx_pool = VK_NULL_HANDLE;
    LOG_DEBUG("vulkan command pool kill");

    re.vkDestroyDevice(core->logic_dvc, core->alloc);
    LOG_DEBUG("vulkan device kill");

#if DEBUG
    re.vkDestroyDebugUtilsMessengerEXT(core->instance, core->util_dbg,
                                       core->alloc);
    LOG_DEBUG("vulkan debugger kill");
#endif

    re.vkDestroyInstance(core->instance, core->alloc);
    LOG_DEBUG("vulkan core kill");
}

bool re_memalloc(vk_core_t *core, VkMemoryRequirements *memory_req,
                 VkMemoryPropertyFlags flags, VkDeviceMemory *out,
                 vram_tag_t tag) {

    if ((core->memories.total_allocated + memory_req->size) >
        core->memories.budget) {
        LOG_ERROR("VRAM Budget exceeded! Could not allocate %llu bytes. "
                  "Total: %llu / %llu",
                  memory_req->size, core->memories.total_allocated,
                  core->memories.budget);
        return false;
    }

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_req->size;
    alloc_info.memoryTypeIndex =
        find_mem_type(core, memory_req->memoryTypeBits, flags);

    if (alloc_info.memoryTypeIndex == UINT32_MAX) {
        LOG_ERROR("Invalid memory type index returned!");
        return false;
    }

    VkResult res =
        re.vkAllocateMemory(core->logic_dvc, &alloc_info, core->alloc, out);

    if (res == VK_SUCCESS) {
        core->memories.total_allocated += memory_req->size;
        core->memories.tag_alloc_count[tag]++;
        core->memories.tag_alloc[tag] += memory_req->size;

        if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            core->memories.dvc_local_used += memory_req->size;
        } else if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            core->memories.host_visible_used += memory_req->size;
        }

        return true;
    }

    return false;
}

void re_memfree(vk_core_t *core, VkDeviceMemory memory,
                VkMemoryPropertyFlags flags, VkDeviceSize size,
                vram_tag_t tag) {

    re.vkFreeMemory(core->logic_dvc, memory, core->alloc);
    core->memories.total_allocated -= size;
    core->memories.tag_alloc_count[tag]--;
    core->memories.tag_alloc[tag] -= size;

    if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        core->memories.dvc_local_used -= size;
    } else if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        core->memories.host_visible_used -= size;
    }
}

/************************************
 * SWAPCHAIN
 ************************************/
static bool create_surface(vk_swapchain_t *swp, vk_core_t *core,
                           window_t *window) {
    if (swp->surface != VK_NULL_HANDLE) return true;

#if PLATFORM_LINUX
    VkXlibSurfaceCreateInfoKHR surf = {};
    surf.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    surf.dpy = (Display *)window->display;
    surf.window = (Window)window->win;

    CHECK_VK(re.vkCreateXlibSurfaceKHR(core->instance, &surf, core->alloc,
                                       &swp->surface));

#elif PLATFORM_WINDOWS
    VkWin32SurfaceCreateInfoKHR surf = {};
    surf.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surf.hinstance = GetModuleHandle(NULL);
    surf.hwnd = (HWND)window->hwnd;

    CHECK_VK(re.vkCreateWin32SurfaceKHR(core->instance, &surf, core->alloc,
                                        &swp->surface));
#else
#    error "Other platform not supported"
#endif
    LOG_DEBUG("vulkan surface created");
    return true;
}

static bool pick_present_queue(vk_core_t *core, VkSurfaceKHR surface) {
    uint32_t q_count = 0;
    re.vkGetPhysicalDeviceQueueFamilyProperties(core->gpu, &q_count, NULL);
    VkQueueFamilyProperties *families =
        WALLOC(sizeof(VkQueueFamilyProperties) * q_count, MEM_RENDER);
    re.vkGetPhysicalDeviceQueueFamilyProperties(core->gpu, &q_count, families);

    for (uint32_t i = 0; i < q_count; ++i) {
        VkBool32 present_supported = VK_FALSE;
        re.vkGetPhysicalDeviceSurfaceSupportKHR(core->gpu, i, surface,
                                                &present_supported);
        if (present_supported) {
            core->present_idx = i;
            WFREE(families, sizeof(VkQueueFamilyProperties) * q_count,
                  MEM_RENDER);
            return true;
        }
    }

    WFREE(families, sizeof(VkQueueFamilyProperties) * q_count, MEM_RENDER);
    LOG_FATAL("No present queue found");
    return false;
}

static bool query_surface_details(vk_swapchain_t *swp, vk_core_t *core) {
    re.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(core->gpu, swp->surface,
                                                 &swp->caps);

    uint32_t format_count = 0;
    re.vkGetPhysicalDeviceSurfaceFormatsKHR(core->gpu, swp->surface,
                                            &format_count, NULL);
    VkSurfaceFormatKHR *formats =
        WALLOC(sizeof(VkSurfaceFormatKHR) * format_count, MEM_RENDER);
    re.vkGetPhysicalDeviceSurfaceFormatsKHR(core->gpu, swp->surface,
                                            &format_count, formats);
    swp->surface_format = formats[0];
    WFREE(formats, sizeof(VkSurfaceFormatKHR) * format_count, MEM_RENDER);

    uint32_t mode_count = 0;
    re.vkGetPhysicalDeviceSurfacePresentModesKHR(core->gpu, swp->surface,
                                                 &mode_count, NULL);
    VkPresentModeKHR *modes =
        WALLOC(sizeof(VkPresentModeKHR) * mode_count, MEM_RENDER);
    re.vkGetPhysicalDeviceSurfacePresentModesKHR(core->gpu, swp->surface,
                                                 &mode_count, modes);
    swp->present_mode = modes[0];
    WFREE(modes, sizeof(VkPresentModeKHR) * mode_count, MEM_RENDER);

    return true;
}

static void choose_surface_format(vk_swapchain_t *swp) {
    // prefer SRGB if available
    if (swp->surface_format.format == VK_FORMAT_UNDEFINED) {
        swp->surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
        swp->surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }
}

static void choose_present_mode(vk_swapchain_t *swp) {
    // prefer MAILBOX > FIFO
    /*
    if (swp->present_mode != VK_PRESENT_MODE_MAILBOX_KHR) {
        swp->present_mode = VK_PRESENT_MODE_FIFO_KHR;
    }
    */

    // prefer FIFO > MAILBOX
    swp->present_mode = VK_PRESENT_MODE_FIFO_KHR;
}

static void choose_swap_extent(vk_swapchain_t *swp, window_t *window) {
    if (swp->caps.currentExtent.width != INVALID_32) {
        swp->extents = swp->caps.currentExtent;
    } else {
        uint32_t win_width = window->width;
        uint32_t win_height = window->height;
        swp->extents.width = CLAMP(win_width, swp->caps.minImageExtent.width,
                                   swp->caps.maxImageExtent.width);
        swp->extents.height = CLAMP(win_height, swp->caps.minImageExtent.height,
                                    swp->caps.maxImageExtent.height);
    }
}

static bool create_swapchain(vk_swapchain_t *swp, vk_core_t *core,
                             VkSwapchainKHR old_handle) {
    uint32_t image_count = swp->caps.minImageCount + 1;
    if (swp->caps.maxImageCount > 0 && image_count > swp->caps.maxImageCount) {
        image_count = swp->caps.maxImageCount;
    }
    LOG_TRACE("image count from swapchain: %d", image_count);

    VkSwapchainCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = swp->surface;
    info.minImageCount = image_count;
    info.imageFormat = swp->surface_format.format;
    info.imageColorSpace = swp->surface_format.colorSpace;
    info.imageExtent = swp->extents;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queue_indices[] = {core->graphic_idx, core->present_idx};
    if (core->graphic_idx != core->present_idx) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = queue_indices;
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    info.preTransform = swp->caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = swp->present_mode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = old_handle;

    CHECK_VK(re.vkCreateSwapchainKHR(core->logic_dvc, &info, core->alloc,
                                     &swp->handle));

    swp->image_format = swp->surface_format.format;
    return true;
}

static bool create_sw_image_views(vk_swapchain_t *swp, vk_core_t *core) {
    re.vkGetSwapchainImagesKHR(core->logic_dvc, swp->handle, &swp->image_count,
                               NULL);
    swp->images = WALLOC(sizeof(VkImage) * swp->image_count, MEM_RENDER);

    re.vkGetSwapchainImagesKHR(core->logic_dvc, swp->handle, &swp->image_count,
                               swp->images);
    swp->img_views = WALLOC(sizeof(VkImageView) * swp->image_count, MEM_RENDER);
    for (uint32_t i = 0; i < swp->image_count; ++i) {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = swp->images[i];
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = swp->image_format;
        info.components =
            (VkComponentMapping){.a = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .r = VK_COMPONENT_SWIZZLE_IDENTITY};

        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;

        CHECK_VK(re.vkCreateImageView(core->logic_dvc, &info, core->alloc,
                                      &swp->img_views[i]));
    }
    return true;
}

static bool create_image_view(vk_image_t *out, vk_core_t *core, VkFormat format,
                              VkImageAspectFlags flags) {
    if (out->handle == VK_NULL_HANDLE) {
        LOG_ERROR("cannot create view for null image!");
        return false;
    }
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = out->handle;
    info.format = format;
    info.components = (VkComponentMapping){.a = VK_COMPONENT_SWIZZLE_IDENTITY,
                                           .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                           .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                           .r = VK_COMPONENT_SWIZZLE_IDENTITY};

    info.subresourceRange.aspectMask = flags;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;

    CHECK_VK(
        re.vkCreateImageView(core->logic_dvc, &info, core->alloc, &out->view));

    return true;
}

static void destroy_image_views(vk_swapchain_t *swp, vk_core_t *core) {
    for (uint32_t i = 0; i < swp->image_count; ++i) {
        re.vkDestroyImageView(core->logic_dvc, swp->img_views[i], core->alloc);
    }
    WFREE(swp->img_views, sizeof(VkImageView) * swp->image_count, MEM_RENDER);
    WFREE(swp->images, sizeof(VkImage) * swp->image_count, MEM_RENDER);
}

bool swapchain_init(vk_swapchain_t *swp, vk_core_t *core, window_t *window,
                    VkSwapchainKHR old_handle) {
    if (!create_surface(swp, core, window)) return false;
    if (!pick_present_queue(core, swp->surface)) return false;
    if (!query_surface_details(swp, core)) return false;
    choose_surface_format(swp);
    choose_present_mode(swp);
    choose_swap_extent(swp, window);
    if (!create_swapchain(swp, core, old_handle)) return false;

    // image swapchain
    if (!create_sw_image_views(swp, core)) return false;

    // color attachment
    if (!image_init(&swp->color_attach, core, swp->image_format,
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT, swp->extents.width,
                    swp->extents.height, true, RE_RENDER_TARGET)) {
        LOG_ERROR("failed to create color attachment swapchain");
        return false;
    }

    // depth attachment
    if (!image_init(&swp->depth_attach, core, core->default_depth_format,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_IMAGE_ASPECT_DEPTH_BIT, swp->extents.width,
                    swp->extents.height, true, RE_DEPTH_TARGET)) {
        LOG_ERROR("failed to create depth attachment swapchain");
        return false;
    }

    if (old_handle && old_handle != VK_NULL_HANDLE) {
        LOG_DEBUG("vulkan swapchain initialize with old handle: %p",
                  (void *)old_handle);
    } else {
        LOG_DEBUG("vulkan swapchain initialize first-time");
    }

    return true;
}

void swapchain_kill(vk_swapchain_t *swp, vk_core_t *core) {
    image_kill(&swp->depth_attach, core, RE_DEPTH_TARGET);
    image_kill(&swp->color_attach, core, RE_RENDER_TARGET);

    destroy_image_views(swp, core);

    if (swp->handle != VK_NULL_HANDLE) {
        re.vkDestroySwapchainKHR(core->logic_dvc, swp->handle, core->alloc);
        swp->handle = VK_NULL_HANDLE;
        LOG_DEBUG("vulkan swapchain kill");
    }

    if (swp->surface != VK_NULL_HANDLE) {
        re.vkDestroySurfaceKHR(core->instance, swp->surface, core->alloc);
        swp->surface = VK_NULL_HANDLE;
        LOG_DEBUG("vulkan surface kill");
    }
}

bool swapchain_reinit(vk_swapchain_t *swp, vk_core_t *core, window_t *window) {
    VkSwapchainKHR old_swapchain = swp->handle;

    if (old_swapchain != VK_NULL_HANDLE) {
        image_kill(&swp->depth_attach, core, RE_DEPTH_TARGET);
        image_kill(&swp->color_attach, core, RE_RENDER_TARGET);
        destroy_image_views(swp, core);
    }

    if (!swapchain_init(swp, core, window, old_swapchain)) return false;

    if (old_swapchain != VK_NULL_HANDLE) {
        re.vkDestroySwapchainKHR(core->logic_dvc, old_swapchain, core->alloc);
    }

    return true;
}

bool image_init(vk_image_t *out, vk_core_t *core, VkFormat format,
                VkImageUsageFlags usage, VkImageAspectFlags flags,
                uint32_t width, uint32_t height, bool create_view,
                vram_tag_t tag) {
    out->width = width;
    out->height = height;
    out->format = format;
    out->handle = VK_NULL_HANDLE;
    out->memory = VK_NULL_HANDLE;
    out->view = VK_NULL_HANDLE;

    VkImageCreateInfo img_info = {};
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.extent.width = width;
    img_info.extent.height = height;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.format = format;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_info.usage = usage;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    CHECK_VK(re.vkCreateImage(core->logic_dvc, &img_info, core->alloc,
                              &out->handle));

    VkMemoryRequirements mem_req;
    re.vkGetImageMemoryRequirements(core->logic_dvc, out->handle, &mem_req);

    out->size = mem_req.size;

    if (re_memalloc(core, &mem_req, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    &out->memory, tag)) {
        CHECK_VK(
            re.vkBindImageMemory(core->logic_dvc, out->handle, out->memory, 0));
    } else {
        re.vkDestroyImage(core->logic_dvc, out->handle, core->alloc);
        re_memfree(core, out->memory, flags, out->size, tag);
        return false;
    }

    if (create_view) {
        out->view = 0;
        create_image_view(out, core, format, flags);
    }

    return true;
}

void image_kill(vk_image_t *image, vk_core_t *core, vram_tag_t tag) {
    if (image->view) {
        re.vkDestroyImageView(core->logic_dvc, image->view, core->alloc);
        image->view = 0;
    }
    if (image->memory) {
        re_memfree(core, image->memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                   image->size, tag);
        image->memory = VK_NULL_HANDLE;
    }
    if (image->handle) {
        re.vkDestroyImage(core->logic_dvc, image->handle, core->alloc);
        image->handle = 0;
    }
    image->size = 0;
}

void image_transition_layout(vk_core_t *core, vk_cmdbuffer_t *cmd,
                             vk_image_t *image, VkFormat *format,
                             VkImageLayout old_layout,
                             VkImageLayout new_layout) {
    (void)format;
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = (uint32_t)core->graphic_idx;
    barrier.dstQueueFamilyIndex = (uint32_t)core->graphic_idx;
    barrier.image = image->handle;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        LOG_FATAL("Unsupported layout transition");
        return;
    }

    re.vkCmdPipelineBarrier(cmd->handle, src_stage, dst_stage, 0, 0, 0, 0, 0, 1,
                            &barrier);
}

void image_copy_buffer(vk_core_t *core, vk_image_t *image, VkBuffer buffer,
                       vk_cmdbuffer_t *cmd) {
    (void)core;
    VkBufferImageCopy region;
    memset(&region, 0, sizeof(VkBufferImageCopy));

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = image->width;
    region.imageExtent.height = image->height;
    region.imageExtent.depth = 1;

    re.vkCmdCopyBufferToImage(cmd->handle, buffer, image->handle,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

/************************************
 * RENDERPASS
 ************************************/
bool renderpass_init(vk_core_t *core, vk_renderpass_t *rpass,
                     vk_swapchain_t *swap, float depth, uint32_t stencil,
                     uint8_t clear_flag, bool prev_pass, bool next_pass,
                     VkClearColorValue clear_color) {
    rpass->clear_flag = clear_flag;
    rpass->depth = depth;
    rpass->stencil = stencil;
    rpass->prev_pass = prev_pass;
    rpass->next_pass = next_pass;
    rpass->clear_color = clear_color;

    VkAttachmentDescription attachments[2];
    uint32_t attach_count = 0;

    /*** Color Attachment ***/
    bool do_clear_color = (rpass->clear_flag & COLOR_BUFFER) != 0;
    VkAttachmentDescription color = {};
    color.format = swap->image_format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = do_clear_color ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                  : VK_ATTACHMENT_LOAD_OP_LOAD;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = prev_pass ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                                    : VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = next_pass ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                                  : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[attach_count++] = color;

    VkAttachmentReference color_ref = {};
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    /*** Depth Attachment ***/
    VkAttachmentReference depth_ref = {};
    bool do_clear_depth = (rpass->clear_flag & DEPTH_BUFFER) != 0;
    if (do_clear_depth) {
        VkAttachmentDescription depth = {};
        depth.format = core->default_depth_format;
        depth.samples = VK_SAMPLE_COUNT_1_BIT;
        depth.loadOp = do_clear_depth ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                      : VK_ATTACHMENT_LOAD_OP_LOAD;
        depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[attach_count++] = depth;

        depth_ref.attachment = 1;
        depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    /*** Subpass ***/
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;
    subpass.pDepthStencilAttachment = do_clear_depth ? &depth_ref : NULL;

    VkSubpassDependency depend[2];
    uint32_t depend_count = 1;

    depend[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    depend[0].dstSubpass = 0;
    depend[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    depend[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    depend[0].srcAccessMask = 0;
    depend[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    depend[0].dependencyFlags = 0;

    if (next_pass) {
        depend[depend_count].srcSubpass = 0;
        depend[depend_count].dstSubpass = VK_SUBPASS_EXTERNAL;
        depend[depend_count].srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        depend[depend_count].srcAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        depend[depend_count].dstStageMask =
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        depend[depend_count].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        depend_count++;
    }

    /*** Renderpass ***/
    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.attachmentCount = attach_count;
    rp_info.pAttachments = attachments;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = depend_count;
    rp_info.pDependencies = depend;

    CHECK_VK(re.vkCreateRenderPass(core->logic_dvc, &rp_info, core->alloc,
                                   &rpass->handle));
    return true;
}

void renderpass_kill(vk_core_t *core, vk_renderpass_t *rpass) {
    re.vkDestroyRenderPass(core->logic_dvc, rpass->handle, core->alloc);
}

void renderpass_begin(vk_renderpass_t *rpass, VkCommandBuffer cmd,
                      VkFramebuffer framebuffer, VkExtent2D extent) {
    VkClearValue clvl[2];
    uint32_t clear_count = 0;

    if (rpass->clear_flag & COLOR_BUFFER) {
        clvl[clear_count].color = rpass->clear_color;
        clear_count++;
    }
    if ((rpass->clear_flag & DEPTH_BUFFER) ||
        (rpass->clear_flag & STENCIL_BUFFER)) {
        clvl[clear_count].depthStencil =
            (VkClearDepthStencilValue){.depth = rpass->depth,
                                       .stencil = rpass->stencil};
        clear_count++;
    }

    VkRenderPassBeginInfo rp_begin = {};
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.renderPass = rpass->handle;
    rp_begin.framebuffer = framebuffer;
    rp_begin.renderArea.offset = (VkOffset2D){0, 0};
    rp_begin.renderArea.extent = extent;
    rp_begin.clearValueCount = clear_count;
    rp_begin.pClearValues = clvl;

    re.vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
}

void renderpass_end(vk_renderpass_t *rpass, VkCommandBuffer cmd) {
    (void)rpass;
    re.vkCmdEndRenderPass(cmd);
}

/************************************
 * COMMANDBUFFER
 ************************************/
bool cmdbuff_alloc(vk_core_t *core, vk_cmdbuffer_t *cmd, VkCommandPool pool) {
    memset(cmd, 0, sizeof(*cmd));

    if (core->gfx_pool == VK_NULL_HANDLE) {
        LOG_ERROR("Command pool is null!");
        return false;
    }

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    CHECK_VK(re.vkAllocateCommandBuffers(core->logic_dvc, &alloc_info,
                                         &cmd->handle));
    return true;
}

void cmdbuff_free(vk_core_t *core, vk_cmdbuffer_t *cmd, VkCommandPool pool) {
    re.vkFreeCommandBuffers(core->logic_dvc, pool, 1, &cmd->handle);
    cmd->handle = VK_NULL_HANDLE;
}

bool cmdbuff_begin(vk_cmdbuffer_t *cmd, cmd_usage_t usage) {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = usage;

    CHECK_VK(re.vkBeginCommandBuffer(cmd->handle, &begin_info));
    return true;
}

bool cmdbuff_reset(vk_cmdbuffer_t *cmd) {
    CHECK_VK(re.vkResetCommandBuffer(cmd->handle, 0));
    return true;
}

void cmdbuff_end(vk_cmdbuffer_t *cmd) {
    CHECK_VK(re.vkEndCommandBuffer(cmd->handle));
}

void cmdbuff_temp_init(vk_core_t *core, vk_cmdbuffer_t *cmd,
                       VkCommandPool pool) {
    cmdbuff_alloc(core, cmd, pool);
    cmdbuff_begin(cmd, SUBMIT_ONE_TIME);
}

void cmdbuff_temp_kill(vk_core_t *core, vk_cmdbuffer_t *cmd, VkCommandPool pool,
                       VkQueue queue) {
    cmdbuff_end(cmd);

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd->handle;
    CHECK_VK(re.vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));
    CHECK_VK(re.vkQueueWaitIdle(queue));

    cmdbuff_free(core, cmd, pool);
}

/************************************
 * BUFFER
 ************************************/
bool buffer_init(vk_core_t *core, vk_buffer_t *out, VkBufferUsageFlags usage,
                 VkDeviceSize size, VkMemoryPropertyFlags mem_prop,
                 vram_tag_t tag) {
    memset(out, 0, sizeof(vk_buffer_t));

    VkBufferCreateInfo bf_info = {};
    bf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bf_info.size = size;
    bf_info.usage = usage;
    bf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CHECK_VK(re.vkCreateBuffer(core->logic_dvc, &bf_info, core->alloc,
                               &out->handle));

    VkMemoryRequirements mem_req;
    re.vkGetBufferMemoryRequirements(core->logic_dvc, out->handle, &mem_req);
    out->size = mem_req.size;
    out->is_locked = false;
    out->mapped = NULL;

    if (re_memalloc(core, &mem_req, mem_prop, &out->memory, tag)) {
        CHECK_VK(re.vkBindBufferMemory(core->logic_dvc, out->handle,
                                       out->memory, 0));
    } else {
        re.vkDestroyBuffer(core->logic_dvc, out->handle, core->alloc);
        out->handle = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

void buffer_kill(vk_core_t *core, vk_buffer_t *buffer,
                 VkMemoryPropertyFlags mem_prop, vram_tag_t tag) {
    if (buffer->memory) {
        re_memfree(core, buffer->memory, mem_prop, buffer->size, tag);
        buffer->memory = 0;
    }
    if (buffer->handle) {
        re.vkDestroyBuffer(core->logic_dvc, buffer->handle, core->alloc);
        buffer->handle = VK_NULL_HANDLE;
    }

    buffer->size = 0;
    buffer->is_locked = false;
    buffer->mapped = 0;
}

void buffer_bind(vk_core_t *core, vk_buffer_t *buffer, VkDeviceSize offset) {
    re.vkBindBufferMemory(core->logic_dvc, buffer->handle, buffer->memory,
                          offset);
}

void buffer_copy(vk_core_t *core, VkBuffer src, VkBuffer dst,
                 VkDeviceSize src_offset, VkDeviceSize dst_offset,
                 VkDeviceSize size, VkCommandPool pool, VkQueue queue) {
    vk_cmdbuffer_t temp;
    cmdbuff_temp_init(core, &temp, pool);

    VkBufferCopy copy_region = {};
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;

    re.vkCmdCopyBuffer(temp.handle, src, dst, 1, &copy_region);
    cmdbuff_temp_kill(core, &temp, pool, queue);
}

void buffer_load(vk_core_t *core, vk_buffer_t *buffer, VkDeviceSize offset,
                 VkDeviceSize size, const void *data) {
    void *mapped_ptr;
    re.vkMapMemory(core->logic_dvc, buffer->memory, offset, size, 0,
                   &mapped_ptr);

    memcpy(mapped_ptr, data, size);
    re.vkUnmapMemory(core->logic_dvc, buffer->memory);
}

/************************************
 * PIPELINE
 ************************************/
bool pipeline_init(vk_core_t *core, vk_pipeline_t *pipeline,
                   vk_renderpass_t *rpass, const vk_pipeline_desc_t *desc,
                   pipe_config_t config) {
    /*** viewport state ***/
    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    /*** multisample state ***/
    VkPipelineMultisampleStateCreateInfo multi_sample_info = {};
    multi_sample_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multi_sample_info.sampleShadingEnable = VK_FALSE;
    multi_sample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multi_sample_info.minSampleShading = 1.0f;

    /*** raster state ***/
    VkPipelineRasterizationStateCreateInfo raster_info = {};
    raster_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_info.polygonMode =
        config.wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    raster_info.cullMode = config.cull_mode;
    raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster_info.lineWidth = 1.0f;

    /*** depth stencil state ***/
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = config.depth_test;
    depth_stencil.depthWriteEnable = config.depth_write;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;

    /*** color blend state ***/
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,

        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,

        .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
    };

    VkPipelineColorBlendStateCreateInfo color_blend_info = {};
    color_blend_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY;

    /*** vertex input state ***/
    VkVertexInputBindingDescription bind_desc;
    bind_desc.binding = 0;
    bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bind_desc.stride = desc->vertex_stride;

    /* Attributes */
    VkPipelineVertexInputStateCreateInfo vert_info = {};
    vert_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vert_info.vertexBindingDescriptionCount = 1;
    vert_info.pVertexBindingDescriptions = &bind_desc;
    vert_info.vertexAttributeDescriptionCount = desc->attribute_count;
    vert_info.pVertexAttributeDescriptions = desc->attrs;

    /*** input assembly state ***/
    VkPipelineInputAssemblyStateCreateInfo input_asm = {};
    input_asm.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_asm.primitiveRestartEnable = VK_FALSE;

    /*** push constant ***/
    VkPushConstantRange push_const[desc->push_constant_count];
    for (uint32_t i = 0; i < desc->push_constant_count; ++i) {
        push_const[i].stageFlags = desc->push_consts[i].stageFlags;
        push_const[i].offset = desc->push_consts[i].offset;
        push_const[i].size = desc->push_consts[i].size;
    }

    /*** pipeline layout ***/
    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = desc->desc_layout_count;
    layout_info.pSetLayouts =
        desc->desc_layout_count > 0 ? desc->desc_layouts : NULL;
    layout_info.pushConstantRangeCount = desc->push_constant_count;
    layout_info.pushConstantRangeCount = desc->push_constant_count;
    layout_info.pPushConstantRanges =
        desc->push_constant_count > 0 ? push_const : NULL;

    CHECK_VK(re.vkCreatePipelineLayout(core->logic_dvc, &layout_info,
                                       core->alloc, &pipeline->layout));

    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                       VK_DYNAMIC_STATE_SCISSOR,
                                       VK_DYNAMIC_STATE_LINE_WIDTH};

    VkPipelineDynamicStateCreateInfo dyn_info = {};
    dyn_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dyn_info.dynamicStateCount = 3;
    dyn_info.pDynamicStates = dynamic_states;

    /*** graphic pipeline ***/
    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.layout = pipeline->layout;
    pipeline_info.renderPass = rpass->handle;
    pipeline_info.stageCount = desc->stage_count;
    pipeline_info.pStages = desc->stages;
    pipeline_info.pVertexInputState = &vert_info;
    pipeline_info.pInputAssemblyState = &input_asm;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &raster_info;
    pipeline_info.pMultisampleState = &multi_sample_info;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.pColorBlendState = &color_blend_info;
    pipeline_info.pDynamicState = &dyn_info;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.subpass = 0;

    CHECK_VK(re.vkCreateGraphicsPipelines(core->logic_dvc, VK_NULL_HANDLE, 1,
                                          &pipeline_info, core->alloc,
                                          &pipeline->handle));

    LOG_DEBUG("vulkan pipeline initialize");
    return true;
}

void pipeline_bind(vk_pipeline_t *pipeline, VkCommandBuffer cmdbuffer,
                   VkPipelineBindPoint bind_point) {
    re.vkCmdBindPipeline(cmdbuffer, bind_point, pipeline->handle);
}

void pipeline_kill(vk_core_t *core, vk_pipeline_t *pipeline) {
    if (pipeline) {
        if (pipeline->handle) {
            re.vkDestroyPipeline(core->logic_dvc, pipeline->handle,
                                 core->alloc);
            pipeline->handle = VK_NULL_HANDLE;
        }

        if (pipeline->layout) {
            re.vkDestroyPipelineLayout(core->logic_dvc, pipeline->layout,
                                       core->alloc);
            pipeline->layout = 0;
        }
        LOG_DEBUG("vulkan pipeline kill");
    }
}

/************************************
 * MATERIAL
 ************************************/
static VkShaderModule vk_shader_create(vk_core_t *core, const void *bytecode,
                                       VkDeviceSize size) {
    VkShaderModuleCreateInfo info =
        {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
         .codeSize = size,
         .pCode = (const uint32_t *)bytecode};

    VkShaderModule module;
    if (re.vkCreateShaderModule(core->logic_dvc, &info, core->alloc, &module) !=
        VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return module;
}

static void vk_shader_destroy(vk_core_t *core, VkShaderModule module) {
    re.vkDestroyShaderModule(core->logic_dvc, module, core->alloc);
}

static bool set_shader(vk_core_t *core, vk_material_t *material,
                       const char *name) {
    char vert_path[MAX_PATH];
    char frag_path[MAX_PATH];
    snprintf(vert_path, sizeof(vert_path), "%s.vert.spv", name);
    snprintf(frag_path, sizeof(frag_path), "%s.frag.spv", name);

    uint64_t vert_size = 0, frag_size = 0;
    void *vert_code = read_file_binary(vert_path, &vert_size);
    void *frag_code = read_file_binary(frag_path, &frag_size);

    if (!vert_code || !frag_code) {
        if (vert_code) WFREE(vert_code, vert_size, MEM_RESOURCE);
        if (frag_code) WFREE(frag_code, frag_size, MEM_RESOURCE);
        return false;
    }

    // Store directly in the bundle
    material->shaders.vert = vk_shader_create(core, vert_code, vert_size);
    material->shaders.frag = vk_shader_create(core, frag_code, frag_size);
    material->shaders.entry_point = "main";

    WFREE(vert_code, vert_size, MEM_RESOURCE);
    WFREE(frag_code, frag_size, MEM_RESOURCE);

    return (material->shaders.vert != VK_NULL_HANDLE &&
            material->shaders.frag != VK_NULL_HANDLE);
}

static void unset_shader(vk_core_t *core, vk_material_t *material) {
    if (!material) return;

    if (material->shaders.vert) {
        vk_shader_destroy(core, material->shaders.vert);
        material->shaders.vert = VK_NULL_HANDLE;
    }
    if (material->shaders.frag) {
        vk_shader_destroy(core, material->shaders.frag);
        material->shaders.frag = VK_NULL_HANDLE;
    }
}

/*
static bool set_material_texture(vk_material_t *mat, const char *path) {
    char ext_path[MAX_PATH];
    snprintf(ext_path, sizeof(ext_path), "%s.mat", path);

    uint64_t size = 0;
    char *text = read_file_text(ext_path, &size);
    if (!text) return false;

    char *line = text;
    while (line && *line) {
        char *next = strchr(line, '\n');
        if (next) *next++ = 0;

        if (strncmp(line, "name=", 5) == 0) {
            strncpy(mat->data.name, line + 5, sizeof(mat->data.name) - 1);
            mat->data.name[sizeof(mat->data.name) - 1] = '\0';
        } else if (strncmp(line, "color=", 6) == 0) {
            if (sscanf(line + 6, "%f %f %f %f", &mat->data.color.comp1.x,
                       &mat->data.color.comp1.y, &mat->data.color.comp1.z,
                       &mat->data.color.comp1.w) != 4) {
                LOG_WARN("Invalid color in material '%s'", mat->data.name);
            }
        } else if (strncmp(line, "texture=", 8) == 0) {
            strncpy(mat->data.texture_name, line + 8,
                    sizeof(mat->data.texture_name) - 1);
            mat->data.texture_name[sizeof(mat->data.texture_name) - 1] = '\0';
            mat->data.has_texture = 1;
        }
        line = next;
    }

    WFREE(text, size + 1, MEM_RESOURCE);
    LOG_INFO("Material CPU loaded: %s (texture=%s)", mat->data.name,
             mat->data.has_texture ? mat->data.texture_name : "none");

    return true;
}
*/

static bool set_material_descriptors(vk_core_t *core, vk_material_t *mat) {
    // Allocate global descriptor
    for (uint32_t i = 0; i < FRAME_FLIGHT; i++) {
        void *map = NULL;
        VkResult res = re.vkMapMemory(core->logic_dvc, mat->buffers[i].memory,
                                      0, sizeof(vk_camera_data_t), 0, &map);

        if (res == VK_SUCCESS) {
            mat->buffers[i].mapped = map;
        } else {
            mat->buffers[i].mapped = NULL;
            LOG_ERROR("Failed to map UBO buffer %u", i);
        }

        VkDescriptorSetAllocateInfo glob_alloc = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = mat->global_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &mat->global_layout,
        };

        if (re.vkAllocateDescriptorSets(core->logic_dvc, &glob_alloc,
                                        &mat->global_sets[i]) != VK_SUCCESS) {
            LOG_ERROR("Failed to allocate global descriptor set %u", i);
            return false;
        }

        VkDescriptorBufferInfo glob_buffer = {.buffer = mat->buffers[i].handle,
                                              .offset = 0,
                                              .range =
                                                  sizeof(vk_camera_data_t)};

        VkWriteDescriptorSet glob_write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mat->global_sets[i],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &glob_buffer,
        };
        re.vkUpdateDescriptorSets(core->logic_dvc, 1, &glob_write, 0, NULL);
    }

    for (uint32_t i = 0; i < FRAME_FLIGHT; ++i) {
        // Allocate object descriptor
        void *map = NULL;
        VkResult res =
            re.vkMapMemory(core->logic_dvc, mat->obj_buffers[i].memory, 0,
                           sizeof(vk_object_data_t), 0, &map);

        if (res == VK_SUCCESS) {
            mat->obj_buffers[i].mapped = map;
        } else {
            mat->obj_buffers[i].mapped = NULL;
            LOG_ERROR("Failed to map object buffer");
        }

        VkDescriptorSetAllocateInfo obj_alloc = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = mat->object_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &mat->object_layout,
        };

        if (re.vkAllocateDescriptorSets(core->logic_dvc, &obj_alloc,
                                        &mat->object_set[i]) != VK_SUCCESS) {
            LOG_ERROR("Failed to allocate object descriptor set");
            return false;
        }

        VkDescriptorBufferInfo obj_buffer = {.buffer =
                                                 mat->obj_buffers[i].handle,
                                             .offset = 0,
                                             .range = sizeof(vk_object_data_t)};

        VkWriteDescriptorSet obj_write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mat->object_set[i],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &obj_buffer,
        };
        re.vkUpdateDescriptorSets(core->logic_dvc, 1, &obj_write, 0, NULL);
    }
    return true;
}

static bool set_material_pipeline(vk_core_t *core, vk_material_t *mat,
                                  vk_renderpass_t *rpass) {
    VkPipelineShaderStageCreateInfo stages[2] =
        {{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = mat->shaders.vert,
          .pName = mat->shaders.entry_point},
         {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = mat->shaders.frag,
          .pName = mat->shaders.entry_point}};

    VkPushConstantRange push_constants[] = {
        {.stageFlags =
             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
         .offset = 0,
         .size = sizeof(mat4)},
    };

    VkVertexInputAttributeDescription attrs[] =
        {{.location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = offsetof(vertex_3d, position)},
         {.location = 1,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = offsetof(vertex_3d, texcoord)}};

    VkDescriptorSetLayout layouts[2] = {mat->global_layout, mat->object_layout};

    vk_pipeline_desc_t pipeline_desc = {.stages = stages,
                                        .stage_count = 2,
                                        .desc_layouts = layouts,
                                        .desc_layout_count = 2,
                                        .push_consts = push_constants,
                                        .push_constant_count = 1,
                                        .attrs = attrs,
                                        .attribute_count = 2,
                                        .vertex_stride = sizeof(vertex_3d)};

    pipe_config_t config = {.wireframe = false,
                            .depth_test = true,
                            .depth_write = true,
                            .cull_mode = VK_CULL_MODE_BACK_BIT};

    if (!pipeline_init(core, &mat->pipelines, rpass, &pipeline_desc, config)) {
        LOG_ERROR("Pipeline cannot be created");
        return false;
    }

    return true;
}

bool material_world_init(vk_core_t *core, vk_renderpass_t *rpass,
                         vk_material_t *mat, const char *shader_name) {
    memset(mat, 0, sizeof(vk_material_t));
    if (!set_shader(core, mat, shader_name)) return false;

    // Global descriptor
    VkDescriptorSetLayoutBinding glob_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };
    VkDescriptorSetLayoutCreateInfo glob_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &glob_binding,
    };
    CHECK_VK(re.vkCreateDescriptorSetLayout(core->logic_dvc, &glob_layout,
                                            core->alloc, &mat->global_layout));

    VkDescriptorPoolSize glob_size = {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                      .descriptorCount = FRAME_FLIGHT};

    VkDescriptorPoolCreateInfo glob_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = FRAME_FLIGHT,
        .poolSizeCount = 1,
        .pPoolSizes = &glob_size,
    };
    CHECK_VK(re.vkCreateDescriptorPool(core->logic_dvc, &glob_pool, core->alloc,
                                       &mat->global_pool));

    // Object descriptor
    VkDescriptorSetLayoutBinding obj_binding[2] = {};

    obj_binding[0].binding = 0;
    obj_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    obj_binding[0].descriptorCount = 1;
    obj_binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    obj_binding[1].binding = 1;
    obj_binding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    obj_binding[1].descriptorCount = 1;
    obj_binding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo obj_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = obj_binding,
    };
    CHECK_VK(re.vkCreateDescriptorSetLayout(core->logic_dvc, &obj_layout,
                                            core->alloc, &mat->object_layout));

    VkDescriptorPoolSize obj_pool_size[2];

    // TODO: CHANGE VALUE DESCRIPTOR COUNT!!!!!!!
    // This for unifom buffer - binding 0
    obj_pool_size[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    obj_pool_size[0].descriptorCount = VK_MATERIAL_COUNT;

    // This for image sampler - binding 1
    obj_pool_size[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    obj_pool_size[1].descriptorCount =
        VK_SHADER_SAMPLER_COUNT * VK_MATERIAL_COUNT;

    VkDescriptorPoolCreateInfo obj_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = VK_MATERIAL_COUNT,
        .poolSizeCount = 2,
        .pPoolSizes = obj_pool_size,
    };
    CHECK_VK(re.vkCreateDescriptorPool(core->logic_dvc, &obj_pool, core->alloc,
                                       &mat->object_pool));

    for (uint32_t i = 0; i < FRAME_FLIGHT; i++) {
        if (!buffer_init(core, &mat->buffers[i],
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         sizeof(vk_camera_data_t),
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         RE_BUFFER_UNIFORM)) {
            LOG_ERROR("Failed to create global buffer %u", i);
            return false;
        }

        if (!buffer_init(core, &mat->obj_buffers[i],
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         sizeof(vk_object_data_t),
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         RE_BUFFER_UNIFORM)) {
            LOG_ERROR("Failed to create object buffer");
            return false;
        }
    }

    set_material_descriptors(core, mat);
    set_material_pipeline(core, mat, rpass);

    return true;
}

void material_kill(vk_core_t *core, vk_material_t *material) {
    pipeline_kill(core, &material->pipelines);

    VkMemoryPropertyFlags mem_prop = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    for (uint32_t i = 0; i < FRAME_FLIGHT; ++i) {
        buffer_kill(core, &material->buffers[i], mem_prop, RE_BUFFER_UNIFORM);
        buffer_kill(core, &material->obj_buffers[i], mem_prop,
                    RE_BUFFER_UNIFORM);
    }

    re.vkDestroyDescriptorPool(core->logic_dvc, material->global_pool,
                               core->alloc);
    material->global_pool = VK_NULL_HANDLE;
    re.vkDestroyDescriptorPool(core->logic_dvc, material->object_pool,
                               core->alloc);
    material->object_pool = VK_NULL_HANDLE;

    re.vkDestroyDescriptorSetLayout(core->logic_dvc, material->global_layout,
                                    core->alloc);
    material->global_layout = VK_NULL_HANDLE;
    re.vkDestroyDescriptorSetLayout(core->logic_dvc, material->object_layout,
                                    core->alloc);
    material->object_layout = VK_NULL_HANDLE;

    unset_shader(core, material);
}

void material_bind(vk_core_t *core, vk_material_t *mat, texture_data_t *tex,
                   uint32_t index) {
    if (!tex || !tex->data_internal) {
        return;
    }

    vk_texture_t *data = (vk_texture_t *)tex->data_internal;
    if (!mat->needs_update[index]) {
        VkDescriptorImageInfo img_info = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = data->image.view,
            .sampler = data->sampler,
        };

        VkWriteDescriptorSet obj_tex_write = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mat->object_set[index],
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &img_info,
        };

        re.vkUpdateDescriptorSets(core->logic_dvc, 1, &obj_tex_write, 0, NULL);
        mat->needs_update[index] = true;
    }
}
