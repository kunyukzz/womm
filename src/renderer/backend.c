#include "backend.h"
#include "core/memory.h"

#include <string.h> // strcmp

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
