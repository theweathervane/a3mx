/* The few Vulkan declarations FILTER on the SDL GPU renderer needs (shader_sdlgpu.c: librashader.h's
 * Vulkan types, and one pipeline barrier), so the build needs no Vulkan SDK or Vulkan-Headers. Values
 * and layouts as Vulkan 1.0 defines them (vulkan_core.h). Only shader_sdlgpu.c includes it. */
#ifndef VULKAN_H_
#define VULKAN_H_ 1
#define VULKAN_CORE_H_ 1   /* (SDL_vulkan.h then leaves its own typedefs out) */
#include <stdint.h>

#if defined(_WIN32) && !defined(_WIN64)
#define VKAPI_CALL __stdcall
#define VKAPI_PTR VKAPI_CALL
#else
#define VKAPI_CALL
#define VKAPI_PTR
#endif
#define VKAPI_ATTR

#define VK_DEFINE_HANDLE(object) typedef struct object##_T *object;
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(_M_ARM64)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif

VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_HANDLE(VkQueue)
VK_DEFINE_HANDLE(VkCommandBuffer)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImage)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)   /* (SDL_vulkan.h's) */
struct VkAllocationCallbacks;

typedef uint32_t VkFlags;
typedef VkFlags VkAccessFlags;
typedef VkFlags VkPipelineStageFlags;
typedef VkFlags VkDependencyFlags;
typedef VkFlags VkImageAspectFlags;

typedef enum VkFormat { VK_FORMAT_UNDEFINED = 0, VK_FORMAT_R8G8B8A8_UNORM = 37, VK_FORMAT_MAX_ENUM = 0x7FFFFFFF } VkFormat;
typedef enum VkImageLayout {
    VK_IMAGE_LAYOUT_UNDEFINED = 0, VK_IMAGE_LAYOUT_GENERAL = 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5, VK_IMAGE_LAYOUT_MAX_ENUM = 0x7FFFFFFF
} VkImageLayout;
typedef enum VkStructureType { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER = 45, VK_STRUCTURE_TYPE_MAX_ENUM = 0x7FFFFFFF } VkStructureType;
#define VK_QUEUE_FAMILY_IGNORED (~0u)
#define VK_IMAGE_ASPECT_COLOR_BIT 0x00000001u
#define VK_ACCESS_SHADER_READ_BIT 0x00000020u
#define VK_ACCESS_COLOR_ATTACHMENT_READ_BIT 0x00000080u
#define VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT 0x00000100u
#define VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT 0x00000080u
#define VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT 0x00000400u

typedef struct VkImageSubresourceRange {
    VkImageAspectFlags aspectMask;
    uint32_t baseMipLevel, levelCount, baseArrayLayer, layerCount;
} VkImageSubresourceRange;
typedef struct VkImageMemoryBarrier {
    VkStructureType sType;
    const void *pNext;
    VkAccessFlags srcAccessMask, dstAccessMask;
    VkImageLayout oldLayout, newLayout;
    uint32_t srcQueueFamilyIndex, dstQueueFamilyIndex;
    VkImage image;
    VkImageSubresourceRange subresourceRange;
} VkImageMemoryBarrier;

typedef void (VKAPI_PTR *PFN_vkVoidFunction)(void);
typedef PFN_vkVoidFunction (VKAPI_PTR *PFN_vkGetInstanceProcAddr)(VkInstance instance, const char *pName);
typedef PFN_vkVoidFunction (VKAPI_PTR *PFN_vkGetDeviceProcAddr)(VkDevice device, const char *pName);
typedef void (VKAPI_PTR *PFN_vkCmdPipelineBarrier)(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const void *pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount, const void *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier *pImageMemoryBarriers);
#endif
