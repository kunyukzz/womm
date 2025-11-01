#ifndef RENDERER_TYPE_H
#define RENDERER_TYPE_H

#include "core/define.h" // IWYU pragma: keep
#include "loader.h"
#include "core/math/math_type.h"
#include "frontend_type.h"

#define FRAME_FLIGHT 3

#define VK_MATERIAL_COUNT 1024
#define VK_SHADER_SAMPLER_COUNT 1
#define MAX_VK_TEXTURE 3
#define MAX_VK_TEXTURE_DEBUG 1

/************************************
 * SWAPCHAIN
 ************************************/
typedef struct {
    VkImage handle;
    VkImageView view;
    VkFormat format;

    VkDeviceMemory memory;
    VkDeviceSize size;

    uint32_t width;
    uint32_t height;
} vk_image_t;

typedef struct vk_swapchain_t {
    VkImage *images;
    VkImageView *img_views;
    VkFramebuffer *framebuffer;

    VkSurfaceKHR surface;
    VkSwapchainKHR handle;

    VkSurfaceCapabilitiesKHR caps;
    VkSurfaceFormatKHR surface_format;

    VkExtent2D extents;

    VkPresentModeKHR present_mode;
    VkFormat image_format;
    uint32_t image_count;

    vk_image_t color_attach;
    vk_image_t depth_attach;
} vk_swapchain_t;

/************************************
 * RENDERPASS & COMMANDBUFFER
 ************************************/
typedef struct {
    VkRenderPass handle;
    float depth;
    uint32_t stencil;
    uint8_t clear_flag;
    bool prev_pass;
    bool next_pass;
    VkClearColorValue clear_color;
} vk_renderpass_t;

typedef struct {
    VkCommandBuffer handle;
} vk_cmdbuffer_t;

/************************************
 * BUFFER
 ************************************/
typedef struct {
    VkBuffer handle;
    VkDeviceSize size;
    VkDeviceMemory memory;
    bool is_locked;
    void *mapped;
} vk_buffer_t;

/************************************
 * MATERIAL
 ************************************/
typedef struct {
    VkShaderModule vert;
    VkShaderModule frag;
    const char *entry_point;
} vk_shader_t;

typedef struct {
    mat4 proj;
    mat4 view;
    mat4 _reserved00; // padding for some graphics card
    mat4 _reserved01; // padding for some graphics card
} vk_camera_data_t;

typedef struct {
    vec4 diffuse_color;
    vec4 _reserved00; // padding for some graphics card
    vec4 _reserved01; // padding for some graphics card
    vec4 _reserved02; // padding for some graphics card
} vk_object_data_t;

typedef struct {
    const VkPipelineShaderStageCreateInfo *stages;
    uint32_t stage_count;

    VkDescriptorSetLayout *desc_layouts;
    uint32_t desc_layout_count;

    VkPushConstantRange *push_consts;
    uint32_t push_constant_count;

    VkVertexInputAttributeDescription *attrs;
    uint32_t attribute_count;
    uint32_t vertex_stride;
} vk_pipeline_desc_t;

typedef struct {
    VkPipeline handle;
    VkPipelineLayout layout;
} vk_pipeline_t;

typedef struct {
    material_data_t data;

    vk_camera_data_t cam_ubo_data;

    vk_buffer_t buffers;
    vk_shader_t shaders;
    vk_pipeline_t pipelines;

    VkDescriptorSet global_sets;
    VkDescriptorPool global_pool;
    VkDescriptorSetLayout global_layout;

    vk_object_data_t obj_data;

    vk_buffer_t obj_buffers[FRAME_FLIGHT];
    VkDescriptorSet object_set[FRAME_FLIGHT];
    VkDescriptorPool object_pool;
    VkDescriptorSetLayout object_layout;

    VkImageView diffuse_map;
    VkSampler sampler;

    bool needs_update[FRAME_FLIGHT];
    uint32_t current_idx[FRAME_FLIGHT];
} vk_material_t;

/************************************
 * TEXTURE
 ************************************/
typedef struct {
    vk_image_t image;
    VkSampler sampler;
} vk_texture_t;

/************************************
 * CORE
 ************************************/
typedef enum {
    RE_UNKNOWN = 0x00,
    RE_TEXTURE,        // Base color, normal, roughness/metallic maps
    RE_TEXTURE_HDR,    // HDR cubemaps, IBL textures
    RE_TEXTURE_UI,     // UI atlas, fonts
    RE_BUFFER_VERTEX,  // Vertex buffers for meshes
    RE_BUFFER_INDEX,   // Index buffers
    RE_BUFFER_UNIFORM, // Uniform buffers (per-frame, per-object)
    RE_BUFFER_STAGING, // Staging buffers for uploads
    RE_BUFFER_COMPUTE, // SSBOs for compute shaders
    RE_RENDER_TARGET,  // Color attachments, G-Buffers
    RE_DEPTH_TARGET,   // Depth/Stencil attachments
    RE_COUNT
} vram_tag_t;

typedef struct {
#if DEBUG
    VkDebugUtilsMessengerEXT util_dbg;
#endif

    VkInstance instance;
    VkAllocationCallbacks *alloc;

    VkPhysicalDevice gpu;
    VkDevice logic_dvc;
    VkCommandPool gfx_pool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties mem_prop;

    struct {
        VkDeviceSize total_allocated;
        VkDeviceSize budget;
        VkDeviceSize dvc_local_used;
        VkDeviceSize host_visible_used;

        uint64_t tag_alloc_count[RE_COUNT];
        uint64_t tag_alloc[RE_COUNT];
    } memories;

    VkQueue graphic_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;
    VkFormat default_depth_format;

    uint32_t graphic_idx;
    uint32_t present_idx;
    uint32_t transfer_idx;
} vk_core_t;

#endif // RENDERER_TYPE_H
