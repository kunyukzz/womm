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
