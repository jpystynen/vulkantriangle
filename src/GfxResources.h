#ifndef CORE_GFX_RESOURCES_H
#define CORE_GFX_RESOURCES_H

// Copyright (c) 2017 Johannes Pystynen
// This code is licensed under the MIT license (MIT)

#include <assert.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

///////////////////////////////////////////////////////////////////////////////

static const uint32_t s_defaultTimeout = 1000000000; // 1 s

namespace core
{

// Helper for checking the returned result from Vulkan functions
#define CHECK_VK_RESULT_SUCCESS(check_vk_func) \
{ \
const VkResult result = (check_vk_func); \
assert(result == VK_SUCCESS); \
}

///////////////////////////////////////////////////////////////////////////////

class Window;

class GfxResources
{
public:

    struct BufferedFrameResource
    {
        uint32_t bufferCount = 0;
        // current index for buffered handles
        uint32_t bufferIndex = 0;

        // Tries to create c_bufferingCount of vectors
        // and recycles them frame by frame
        std::vector<VkImage> images;
        std::vector<VkImageView> imageViews;
        std::vector<VkFramebuffer> framebuffers;

        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkFence> commandBufferFences;

        // signaled when image is acquired
        VkSemaphore swapchainImageSemaphore;
        // signaled when cmd buffer submit is done
        VkSemaphore cmdBufferSubmitSemaphore;
    };

    GfxResources(Window* const p_window);
    ~GfxResources();

    GfxResources(const GfxResources&) = delete;
    GfxResources& operator=(const GfxResources&) = delete;

    VkDevice getDevice();
    VkSwapchainKHR getSwapchain();
    VkRenderPass getRenderPass();
    VkPipeline getGraphicsPipeline();
    VkQueue getQueue();

    BufferedFrameResource& getBufferedFrameResource();

private:

    const uint32_t c_bufferingCount = 3;
    const char* c_vertexShader      = "shaders/triangle.vert.spv";
    const char* c_fragmentShader    = "shaders/triangle.frag.spv";

    void create();
    void destroy();

    void createInstance();
    void createPhysicalDevice();
    void createSurface();
    void createSwapchain();
    void createRenderPass();
    void createFramebuffer();
    void createGraphicsPipeline();
    void createQueueAndPool();
    void createSemaphores();

    Window* const mp_window = nullptr;

    BufferedFrameResource m_bufferedFrameResource;

    VkInstance m_instance               = nullptr;
    VkPhysicalDevice m_physicalDevice   = nullptr;
    VkDevice m_device                   = nullptr;

    VkSurfaceKHR m_surface      = nullptr;
    VkSwapchainKHR m_swapchain  = nullptr;

    VkFormat m_swapChainImageformat = VK_FORMAT_UNDEFINED;

#ifdef _DEBUG
    VkDebugReportCallbackEXT m_debugReportCallback = nullptr;
#endif

    VkRenderPass m_renderPass           = nullptr;
    VkPipeline m_graphicsPipeline       = nullptr;
    VkPipelineLayout m_pipelineLayout   = nullptr;

    VkQueue m_queue             = nullptr;
    VkCommandPool m_commandPool = nullptr;
    uint32_t m_queueFamilyIndex = ~0u;

    struct Shader
    {
        VkShaderModule vert = nullptr;
        VkShaderModule frag = nullptr;
    };

    Shader m_shader;
};

} // namespace

///////////////////////////////////////////////////////////////////////////////

#endif // CORE_GFX_RESOURCES_H
