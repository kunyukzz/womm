#include "backend_type.h"
#include "platform/window.h"

/************************************
 * CORE
 ************************************/
bool core_init(vk_core_t *core);
void core_kill(vk_core_t *core);

bool re_memalloc(vk_core_t *core, VkMemoryRequirements *memory_req,
                 VkMemoryPropertyFlags flags, VkDeviceMemory *out,
                 vram_tag_t tag);

void re_memfree(vk_core_t *core, VkDeviceMemory memory,
                VkMemoryPropertyFlags flags, VkDeviceSize size, vram_tag_t tag);

/************************************
 * SWAPCHAIN
 ************************************/
bool swapchain_init(vk_swapchain_t *swp, vk_core_t *core, window_t *window,
                    VkSwapchainKHR old_handle);

void swapchain_kill(vk_swapchain_t *swp, vk_core_t *core);

bool swapchain_reinit(vk_swapchain_t *swp, vk_core_t *core, window_t *window);

bool image_init(vk_image_t *out, vk_core_t *core, VkFormat format,
                VkImageUsageFlags usage, VkImageAspectFlags flags,
                uint32_t width, uint32_t height, bool create_view,
                vram_tag_t tag);

void image_kill(vk_image_t *image, vk_core_t *core, vram_tag_t tag);

void image_transition_layout(vk_core_t *core, vk_cmdbuffer_t *cmd,
                             vk_image_t *image, VkFormat *format,
                             VkImageLayout old_layout,
                             VkImageLayout new_layout);

void image_copy_buffer(vk_core_t *core, vk_image_t *image, VkBuffer buffer,
                       vk_cmdbuffer_t *cmd);

/************************************
 * RENDERPASS
 ************************************/
typedef enum {
    NO_FLAG = 0x0,
    COLOR_BUFFER = 0x1,
    DEPTH_BUFFER = 0x2,
    STENCIL_BUFFER = 0x4
} rpass_flag_t;

bool renderpass_init(vk_core_t *core, vk_renderpass_t *rpass,
                     vk_swapchain_t *swap, float depth, uint32_t stencil,
                     uint8_t clear_flag, bool prev_pass, bool next_pass,
                     VkClearColorValue clear_color);

void renderpass_kill(vk_core_t *core, vk_renderpass_t *rpass);

void renderpass_begin(vk_renderpass_t *rpass, VkCommandBuffer cmd,
                      VkFramebuffer framebuffer, VkExtent2D extent);

void renderpass_end(vk_renderpass_t *rpass, VkCommandBuffer cmd);

/************************************
 * COMMANDBUFFER
 ************************************/
typedef enum {
    SUBMIT_ONE_TIME = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    SUBMIT_RENDERPASS_CONTINUE =
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
    SUBMIT_SIMULTANEOUS = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
} cmd_usage_t;

bool cmdbuff_alloc(vk_core_t *core, vk_cmdbuffer_t *cmd, VkCommandPool pool);
void cmdbuff_free(vk_core_t *core, vk_cmdbuffer_t *cmd, VkCommandPool pool);

bool cmdbuff_begin(vk_cmdbuffer_t *cmd, cmd_usage_t usage);
bool cmdbuff_reset(vk_cmdbuffer_t *cmd);
void cmdbuff_end(vk_cmdbuffer_t *cmd);

void cmdbuff_temp_init(vk_core_t *core, vk_cmdbuffer_t *cmd,
                       VkCommandPool pool);

void cmdbuff_temp_kill(vk_core_t *core, vk_cmdbuffer_t *cmd, VkCommandPool pool,
                       VkQueue queue);

/************************************
 * BUFFER
 ************************************/
bool buffer_init(vk_core_t *core, vk_buffer_t *out, VkBufferUsageFlags usage,
                 VkDeviceSize size, VkMemoryPropertyFlags mem_prop,
                 vram_tag_t tag);

void buffer_kill(vk_core_t *core, vk_buffer_t *buffer,
                 VkMemoryPropertyFlags mem_prop, vram_tag_t tag);

void buffer_bind(vk_core_t *core, vk_buffer_t *buffer, VkDeviceSize offset);

void buffer_copy(vk_core_t *core, VkBuffer src, VkBuffer dst,
                 VkDeviceSize src_offset, VkDeviceSize dst_offset,
                 VkDeviceSize size, VkCommandPool pool, VkQueue queue);

void buffer_load(vk_core_t *core, vk_buffer_t *buffer, VkDeviceSize offset,
                 VkDeviceSize size, const void *data);

/************************************
 * PIPELINE
 ************************************/
typedef struct {
    bool wireframe;
    bool depth_test;
    bool depth_write;
    VkCullModeFlags cull_mode;
} pipe_config_t;

bool pipeline_init(vk_core_t *core, vk_pipeline_t *pipeline,
                   vk_renderpass_t *rpass, const vk_pipeline_desc_t *desc,
                   pipe_config_t config);

void pipeline_bind(vk_pipeline_t *pipeline, VkCommandBuffer cmdbuffer,
                   VkPipelineBindPoint bind_point);

void pipeline_kill(vk_core_t *core, vk_pipeline_t *pipeline);

/************************************
 * MATERIAL
 ************************************/
bool material_world_init(vk_core_t *core, vk_renderpass_t *rpass,
                         vk_material_t *mat, const char *shader_name);

void material_kill(vk_core_t *core, vk_material_t *material);

void material_use(vk_material_t *mat, VkCommandBuffer buffer);

void material_world_set(object_bundle_t *obj, VkCommandBuffer buffer,
                        VkPipelineLayout layout);

void material_ui_set(object_bundle_t *obj, VkCommandBuffer buffer,
                     VkPipelineLayout layout);

void material_prepare(vk_core_t *core, vk_material_t *mat, uint32_t frame_idx,
                      object_bundle_t *objects, uint32_t object_count,
                      bool is_world);

void material_bind(vk_material_t *mat, VkCommandBuffer cmds,
                   VkPipelineLayout layout, uint32_t frame_idx);

bool material_debug_ui_init(vk_core_t *core, vk_renderpass_t *rpass,
                            vk_material_t *mat, const char *shader_name);
