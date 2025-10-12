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
