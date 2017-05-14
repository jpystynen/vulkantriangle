// Copyright (c) 2017 Johannes Pystynen
// This code is licensed under the MIT license (MIT)

#include "GfxResources.h"

#include "Utils.h"
#include "Window.h"

#include <assert.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include <vulkan/vulkan.h>

///////////////////////////////////////////////////////////////////////////////

#define DEF_PRINT_DEVICE_PROPERTIES 1

#ifdef _DEBUG
#define DEF_USE_DEBUG_VALIDATION 1
#endif

namespace core
{

#if (DEF_USE_DEBUG_VALIDATION == 1)

static VkBool32 VKAPI_PTR debugCallback(
    VkDebugReportFlagsEXT       /*flags*/,
    VkDebugReportObjectTypeEXT  /*objectType*/,
    uint64_t                    /*object*/,
    size_t                      /*location*/,
    int32_t                     /*messageCode*/,
    const char                  * /*pLayerPrefix*/,
    const char                  *pMessage,
    void*                       /*pUserData*/)
{
    std::cerr << "debug validation: " << pMessage << std::endl;
    assert(false);
    return VK_FALSE;
}

PFN_vkCreateDebugReportCallbackEXT	fvkCreateDebugReportCallbackEXT = nullptr;
PFN_vkDestroyDebugReportCallbackEXT	fvkDestroyDebugReportCallbackEXT = nullptr;

#endif

static VkShaderModule createShaderModule(
    VkDevice device,
    const char* const shaderFile)
{
    assert(device);
    VkShaderModule shaderModule = nullptr;

    std::ifstream file(shaderFile, std::ios::ate | std::ios::binary);
    assert(file.is_open() && "Shader file not found! Correct working dir, shaders compiled?");

    if (file.is_open())
    {
        const size_t fileSize = file.tellg();
        std::vector<char> binaryShader(fileSize);
        file.seekg(0);
        file.read(binaryShader.data(), fileSize);

        const VkShaderModuleCreateInfo shaderModuleCreateInfo =
        {
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,// sType
            nullptr,                                    // pNext
            0,                                          // flags
            binaryShader.size(),                        // codeSize
            (uint32_t*)(binaryShader.data())            // pCode
        };

        CHECK_VK_RESULT_SUCCESS(vkCreateShaderModule(
            device,                 // device
            &shaderModuleCreateInfo,// pCreateInfo
            nullptr,                // pAllocator
            &shaderModule));        // pShaderModule
    }

    return shaderModule;
}

///////////////////////////////////////////////////////////////////////////////

GfxResources::GfxResources(Window* const p_window)
    : mp_window(p_window)
{
    assert(p_window);

    create();
}

GfxResources::~GfxResources()
{
    destroy();
}

void GfxResources::destroy()
{
    vkDeviceWaitIdle(m_device);

    vkDestroyShaderModule(m_device, m_shader.vert, nullptr);
    vkDestroyShaderModule(m_device, m_shader.frag, nullptr);

    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyPipeline(m_device,	m_graphicsPipeline,	nullptr);

    for (uint32_t idx = 0; idx < m_bufferedFrameResource.bufferCount; ++idx)
    {
        vkDestroyImageView(m_device, m_bufferedFrameResource.imageViews[idx], nullptr);
        vkDestroyFramebuffer(m_device, m_bufferedFrameResource.framebuffers[idx], nullptr);
        vkDestroyFence(m_device, m_bufferedFrameResource.commandBufferFences[idx], nullptr);
    }

    vkFreeCommandBuffers(m_device, m_commandPool,
        (uint32_t)(m_bufferedFrameResource.commandBuffers.size()),
        m_bufferedFrameResource.commandBuffers.data()); // not really needed due to vkDestroyCommandPool
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    vkDestroySemaphore(m_device, m_bufferedFrameResource.swapchainImageSemaphore, nullptr);
    vkDestroySemaphore(m_device, m_bufferedFrameResource.cmdBufferSubmitSemaphore, nullptr);

    vkDestroyRenderPass(m_device, m_renderPass,	nullptr);

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyDevice(m_device, nullptr);

#if (DEF_USE_DEBUG_VALIDATION == 1)
    fvkDestroyDebugReportCallbackEXT(m_instance, m_debugReportCallback, nullptr);
#endif

    vkDestroyInstance(m_instance, nullptr);
}

void GfxResources::create()
{
    createInstance();
    createPhysicalDevice();
    createSurface();
    createSwapchain();
    createRenderPass();
    createFramebuffer();
    createGraphicsPipeline();
    createQueueAndPool();
    createSemaphores();
}

void GfxResources::createInstance()
{
    const GlobalVariables& gv = GlobalVariables::getInstance();

    const VkApplicationInfo applicationInfo =
    {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, // sType
        nullptr,                            // pNext
        gv.applicationName.c_str(),         // pApplicationName
        gv.applicationVersion,              // applicationVersion
        gv.engineName.c_str(),              // pEngineName
        gv.engineVersion,                   // engineVersion
        gv.apiVersion,                      // apiVersion
    };

    std::vector<const char*> extensions;
    std::vector<const char*> layers;

    extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
    extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#if (DEF_USE_DEBUG_VALIDATION == 1)
    extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    // this is the most important thing
    layers.emplace_back("VK_LAYER_LUNARG_standard_validation");
#endif

    const VkInstanceCreateInfo instanceCreateInfo =
    {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // sType
        nullptr,                                // pNext
        0,                                      // flags
        &applicationInfo,                       // pApplicationInfo
        (uint32_t)layers.size(),                // enabledLayerCount
        layers.data(),                          // ppEnabledLayerNames
        (uint32_t)extensions.size(),            // enabledExtensionCount
        extensions.data()                       // ppEnabledExtensionNames
    };

    CHECK_VK_RESULT_SUCCESS(vkCreateInstance(
        &instanceCreateInfo,    // pCreateInfo
        nullptr,                // pAllocator
        &m_instance));          // pInstance

#if (DEF_USE_DEBUG_VALIDATION == 1)
    fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)
        vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT");
    assert(fvkCreateDebugReportCallbackEXT);
    fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)
        vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT");
    assert(fvkDestroyDebugReportCallbackEXT);

    const VkDebugReportCallbackCreateInfoEXT reportCallbackCreateInfo =
    {
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,            // sType
        nullptr,                                                            // pNext
        VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,                        // flags
        debugCallback,                                                      // pfnCallback
        nullptr                                                             // pUserData
    };

    CHECK_VK_RESULT_SUCCESS(fvkCreateDebugReportCallbackEXT(
        m_instance,                 // instance
        &reportCallbackCreateInfo,  // pCreateInfo
        nullptr,                    // pAllocator
        &m_debugReportCallback));   // pCallback
#endif
}

void GfxResources::createPhysicalDevice()
{
    uint32_t physicalDeviceCount = 1; // let's try to get one
    CHECK_VK_RESULT_SUCCESS(vkEnumeratePhysicalDevices(
        m_instance,             // instance
        &physicalDeviceCount,   // pPhysicalDeviceCount
        &m_physicalDevice));    // pPhysicalDevices

#if (DEF_PRINT_DEVICE_PROPERTIES == 1)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

        const uint32_t version = physicalDeviceProperties.apiVersion;
        const uint32_t drVersion = physicalDeviceProperties.driverVersion;
        std::cout << "apiVersion:        " << VK_VERSION_MAJOR(version) << "."
            << VK_VERSION_MINOR(version) << "." << VK_VERSION_PATCH(version) << std::endl;
        std::cout << "driverVersion:     " << VK_VERSION_MAJOR(drVersion) << "."
            << VK_VERSION_MINOR(drVersion) << "." << VK_VERSION_PATCH(drVersion) << std::endl;
        std::cout << "vendorID:          " << physicalDeviceProperties.vendorID << std::endl;
        std::cout << "deviceID:          " << physicalDeviceProperties.deviceID << std::endl;
        std::cout << "deviceType:        " << physicalDeviceProperties.deviceType << std::endl;
        std::cout << "deviceName:        " << physicalDeviceProperties.deviceName << std::endl;
    }
#endif

    // not used atm
    //VkPhysicalDeviceFeatures physicalDeviceFeatures;
    //vkGetPhysicalDeviceFeatures(
    //    m_physicalDevice,           // physicalDevice
    //    &physicalDeviceFeatures);   // pFeatures
    //VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    //vkGetPhysicalDeviceMemoryProperties(
    //    m_physicalDevice,                   // physicalDevice
    //    &physicalDeviceMemoryProperties);   // pMemoryProperties

    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
        m_physicalDevice,               // physicalDevice
        &queueFamilyPropertyCount,      // pQueueFamilyPropertyCount
        nullptr);                       // pQueueFamilyProperties

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        m_physicalDevice,               // physicalDevice
        &queueFamilyPropertyCount,      // pQueueFamilyPropertyCount
        queueFamilyProperties.data());  // pQueueFamilyProperties

    for (uint32_t idx = 0; idx < queueFamilyProperties.size(); ++idx)
    {
        // just get the first with graphics queue
        if (queueFamilyProperties[idx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            m_queueFamilyIndex = idx;
            break;
        }
    }
    assert(m_queueFamilyIndex != ~0u);

    // we don't need anything fancy
    VkPhysicalDeviceFeatures requiredDeviceFeatures = {};

    constexpr float queuePriorities[] = { 0.0f };
    const VkDeviceQueueCreateInfo deviceQueueCreateInfo =
    {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
        nullptr,                                    // pNext
        0,                                          // flags
        m_queueFamilyIndex,                         // queueFamilyIndex
        1,                                          // queueCount
        queuePriorities                             // pQueuePriorities
    };

    std::vector<const char*> extensions;
    extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    const VkDeviceCreateInfo deviceCreateInfo =
    {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,   // sType
        nullptr,                                // pNext
        0,                                      // flags
        1,                                      // queueCreateInfoCount
        &deviceQueueCreateInfo,                 // pQueueCreateInfos
        0,                                      // enabledLayerCount
        nullptr,                                // ppEnabledLayerNames
        (uint32_t)extensions.size(),            // enabledExtensionCount
        extensions.data(),                      // ppEnabledExtensionNames
        &requiredDeviceFeatures                 // pEnabledFeatures
    };

    CHECK_VK_RESULT_SUCCESS(vkCreateDevice(
        m_physicalDevice,   // physicalDevice
        &deviceCreateInfo,  // pCreateInfo
        nullptr,            // pAllocator
        &m_device));        // pDevice
}

void GfxResources::createSurface()
{
    const VkBool32 hasPresentationSupport = vkGetPhysicalDeviceWin32PresentationSupportKHR(
        m_physicalDevice,       // physicalDevice
        m_queueFamilyIndex);    // queueFamilyIndex
    assert(hasPresentationSupport);

    const VkWin32SurfaceCreateInfoKHR surfaceCreateInfo =
    {
        //VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR,
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,    // sType
        nullptr,                                            // pNext
        0,                                                  // flags
        mp_window->getHinstance(),                          // hinstance
        mp_window->getHwnd()                                // hwnd
    };

    CHECK_VK_RESULT_SUCCESS(vkCreateWin32SurfaceKHR(
        m_instance,         // instance
        &surfaceCreateInfo, // pCreateInfo
        nullptr,            // pAllocator
        &m_surface));       // pSurface
}

void GfxResources::createSwapchain()
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    CHECK_VK_RESULT_SUCCESS(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        m_physicalDevice,       // physicalDevice
        m_surface,              // surface
        &surfaceCapabilities)); // pSurfaceCapabilities

    uint32_t surfaceFormatCount = 0;
    CHECK_VK_RESULT_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_physicalDevice,       // physicalDevice
        m_surface,              // surface
        &surfaceFormatCount,    // pSurfaceFormatCount
        nullptr));              // pSurfaceFormats

    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    CHECK_VK_RESULT_SUCCESS(vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_physicalDevice,       // physicalDevice
        m_surface,              // surface
        &surfaceFormatCount,    // pSurfaceFormatCount
        surfaceFormats.data()));// pSurfaceFormats
    assert(surfaceFormats.size() > 0);

    // take the first format and color space
    m_swapChainImageformat = surfaceFormats[0].format;
    const VkColorSpaceKHR colorSpace = surfaceFormats[0].colorSpace;

    uint32_t presentModeCount = 0;
    CHECK_VK_RESULT_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_physicalDevice,   // physicalDevice
        m_surface,          // surface
        &presentModeCount,  // pPresentModeCount
        nullptr));          // pPresentModes

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    CHECK_VK_RESULT_SUCCESS(vkGetPhysicalDeviceSurfacePresentModesKHR(
        m_physicalDevice,       // physicalDevice
        m_surface,              // surface
        &presentModeCount,      // pPresentModeCount
        presentModes.data()));  // pPresentModes
    assert(presentModes.size() > 0);

    // take the first present mode
    const VkPresentModeKHR presentMode = presentModes[0];

    {
        VkBool32 surfaceSupported = VK_FALSE;
        CHECK_VK_RESULT_SUCCESS(vkGetPhysicalDeviceSurfaceSupportKHR(
            m_physicalDevice,
            m_queueFamilyIndex,
            m_surface,
            &surfaceSupported));
        assert(surfaceSupported == VK_TRUE);
    }

    const VkSwapchainCreateInfoKHR swapchainCreateInfo =
    {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,            // sType
        nullptr,                                                // pNext
        0,                                                      // flags
        m_surface,                                              // surface
        c_bufferingCount,                                       // minImageCount
        m_swapChainImageformat,                                 // imageFormat
        colorSpace,                                             // imageColorSpace
        VkExtent2D{ mp_window->getWidth(), mp_window->getHeight() },    // imageExtent
        1,                                                      // imageArrayLayers
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,                    // imageUsage
        VK_SHARING_MODE_EXCLUSIVE,                              // imageSharingMode
        1,                                                      // queueFamilyIndexCount
        &m_queueFamilyIndex,                                    // pQueueFamilyIndices
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,                  // preTransform
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,                      // compositeAlpha
        presentMode,                                            // presentMode
        VK_TRUE,                                                // clipped
        nullptr,                                                // oldSwapchain
    };

    CHECK_VK_RESULT_SUCCESS(vkCreateSwapchainKHR(
        m_device,               // device
        &swapchainCreateInfo,   // pCreateInfo
        nullptr,                // pAllocator
        &m_swapchain));         // pSwapchain

    CHECK_VK_RESULT_SUCCESS(vkGetSwapchainImagesKHR(
        m_device,                               // device
        m_swapchain,                            // swapchain
        &m_bufferedFrameResource.bufferCount,   // pSwapchainImageCount
        nullptr));                              // pSwapchainImages

    m_bufferedFrameResource.images.resize(m_bufferedFrameResource.bufferCount);
    CHECK_VK_RESULT_SUCCESS(vkGetSwapchainImagesKHR(
        m_device,                               // device
        m_swapchain,                            // swapchain
        &m_bufferedFrameResource.bufferCount,   // pSwapchainImageCount
        m_bufferedFrameResource.images.data()));// pSwapchainImages

    constexpr VkImageSubresourceRange imageSubresourceRange =
    {
        VK_IMAGE_ASPECT_COLOR_BIT,  // aspectMask;
        0,                          // baseMipLevel;
        1,                          // levelCount;
        0,                          // baseArrayLayer;
        1,                          // layerCount;
    };
    constexpr VkComponentMapping componentMapping =
    {
        VK_COMPONENT_SWIZZLE_IDENTITY,  // r
        VK_COMPONENT_SWIZZLE_IDENTITY,  // g
        VK_COMPONENT_SWIZZLE_IDENTITY,  // b
        VK_COMPONENT_SWIZZLE_IDENTITY,  // a
    };

    m_bufferedFrameResource.imageViews.resize(m_bufferedFrameResource.bufferCount);
    for (uint32_t idx = 0; idx < m_bufferedFrameResource.imageViews.size(); ++idx)
    {
        const VkImageViewCreateInfo imageViewCreateInfo =
        {
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,   // sType
            nullptr,                                    // pNext
            0,                                          // flags
            m_bufferedFrameResource.images[idx],        // image
            VK_IMAGE_VIEW_TYPE_2D,                      // viewType
            m_swapChainImageformat,                     // format
            componentMapping,                           // components
            imageSubresourceRange                       // subresourceRange;
        };
        CHECK_VK_RESULT_SUCCESS(vkCreateImageView(
            m_device,                                   // device
            &imageViewCreateInfo,                       // pCreateInfo
            nullptr,                                    // pAllocator
            &m_bufferedFrameResource.imageViews[idx])); // pView
    }

    // framebuffers are done later for imageviews

}

void GfxResources::createRenderPass()
{
    const VkAttachmentDescription attachments[] =
    {
        {
            0,                                  // flags
            m_swapChainImageformat,             // format
            VK_SAMPLE_COUNT_1_BIT,              // samples
            VK_ATTACHMENT_LOAD_OP_CLEAR,        // loadOp
            VK_ATTACHMENT_STORE_OP_STORE,       // storeOp
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,    // stencilLoadOp
            VK_ATTACHMENT_STORE_OP_DONT_CARE,   // stencilStoreOp
            VK_IMAGE_LAYOUT_UNDEFINED,          // initialLayout
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR     // finalLayout
        }
    };

    constexpr VkAttachmentReference attachmentReferences[] =
    {
        {
            0,                                          // attachment
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL    // layout
        }
    };

    constexpr VkSubpassDescription subpassDescription =
    {
        0,                              // flags
        VK_PIPELINE_BIND_POINT_GRAPHICS,// pipelineBindPoint
        0,                              // inputAttachmentCount
        nullptr,                        // pInputAttachments
        1,                              // colorAttachmentCount
        &attachmentReferences[0],       // pColorAttachments
        nullptr,                        // pResolveAttachments
        nullptr,                        // pDepthStencilAttachment
        0,                              // preserveAttachmentCount
        nullptr                         // pPreserveAttachments
    };

    const VkRenderPassCreateInfo createInfo =
    {
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,  // sType
        nullptr,                                    // pNext
        0,                                          // flags
        1,                                          // attachmentCount
        &attachments[0],                            // pAttachments
        1,                                          // subpassCount
        &subpassDescription,                        // pSubpasses
        0,                                          // dependencyCount
        nullptr,                                    // pDependencies
    };

    CHECK_VK_RESULT_SUCCESS(vkCreateRenderPass(
        m_device,           // device
        &createInfo,        // pCreateInfo
        nullptr,            // pAllocator
        &m_renderPass));    // pRenderPass
}

void GfxResources::createFramebuffer()
{
    // create framebuffers for swapchain image views
    m_bufferedFrameResource.framebuffers.resize(m_bufferedFrameResource.bufferCount);
    for (uint32_t idx = 0; idx < m_bufferedFrameResource.framebuffers.size(); ++idx)
    {
        const VkFramebufferCreateInfo framebufferCreateInfo =
        {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,  // sType
            nullptr,                                    // pNext
            0,                                          // flags
            m_renderPass,                               // renderPass
            1,                                          // attachmentCount
            &m_bufferedFrameResource.imageViews[idx],   // pAttachments
            mp_window->getWidth(),                      // width
            mp_window->getHeight(),                     // height
            1,                                          // layers
        };

        CHECK_VK_RESULT_SUCCESS(vkCreateFramebuffer(
            m_device,                                       // device
            &framebufferCreateInfo,                         // pCreateInfo
            nullptr,                                        // pAllocator
            &m_bufferedFrameResource.framebuffers[idx]));   // pFramebuffer
    }
}

void GfxResources::createGraphicsPipeline()
{
    m_shader.vert = createShaderModule(m_device, c_vertexShader);
    m_shader.frag = createShaderModule(m_device, c_fragmentShader);

    const VkPipelineShaderStageCreateInfo shaderStageCreateInfo[] =
    {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,    // sType
            nullptr,                                                // pNext
            0,                                                      // flags
            VK_SHADER_STAGE_VERTEX_BIT,                             // stage
            m_shader.vert,                                          // module
            "main",                                                 // pName
            nullptr                                                 // pSpecializationInfo
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,    // sType
            nullptr,                                                // pNext
            0,                                                      // flags
            VK_SHADER_STAGE_FRAGMENT_BIT,                           // stage
            m_shader.frag,                                          // module
            "main",                                                 // pName
            nullptr                                                 // pSpecializationInfo
        }
    };

    constexpr VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,  // sType
        nullptr,                                                    // pNext
        0,                                                          // flags
        0,                                                          // vertexBindingDescriptionCount
        nullptr,                                                    // pVertexBindingDescriptions
        0,                                                          // vertexAttributeDescriptionCount
        nullptr                                                     // pVertexAttributeDescriptions
    };

    constexpr VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,    // sType
        nullptr,                                                        // pNext
        0,                                                              // flags
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                            // topology
        VK_FALSE                                                        // primitiveRestartEnable
    };

    const uint32_t width = mp_window->getWidth();
    const uint32_t height = mp_window->getHeight();

    const VkViewport viewport =
    {
        0.0f,           // x
        0.0f,           // y
        (float)width,   // width
        (float)height,  // height
        0.0f,           // minDepth
        1.0f,           // maxDepth
    };

    const VkRect2D scissor =
    {
        { 0, 0 },           // offset
        { width, height }   // extent
    };

    const VkPipelineViewportStateCreateInfo viewportStateCreateInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,  // sType
        nullptr,                                                // pNext
        0,                                                      // flags
        1,                                                      // viewportCount
        &viewport,                                              // pViewports
        1,                                                      // scissorCount
        &scissor                                                // pScissors
    };

    constexpr VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // sType
        nullptr,                                                    // pNext
        0,                                                          // flags
        VK_FALSE,                                                   // depthClampEnable
        VK_FALSE,                                                   // rasterizerDiscardEnable
        VK_POLYGON_MODE_FILL,                                       // polygonMode
        VK_CULL_MODE_NONE,                                          // cullMode
        VK_FRONT_FACE_COUNTER_CLOCKWISE,                            // frontFace
        VK_FALSE,                                                   // depthBiasEnable
        0.0f,                                                       // depthBiasConstantFactor
        0.0f,                                                       // depthBiasClamp
        0.0f,                                                       // depthBiasSlopeFactor
        1.0f                                                        // lineWidth
    };

    constexpr VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
    {
        VK_FALSE,                                                   // blendEnable
        VK_BLEND_FACTOR_ZERO,                                       // srcColorBlendFactor
        VK_BLEND_FACTOR_ZERO,                                       // dstColorBlendFactor
        VK_BLEND_OP_ADD,                                            // colorBlendOp
        VK_BLEND_FACTOR_ZERO,                                       // srcAlphaBlendFactor
        VK_BLEND_FACTOR_ZERO,                                       // dstAlphaBlendFactor
        VK_BLEND_OP_ADD,                                            // alphaBlendOp
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,  // colorWriteMask
    };

    constexpr VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,   // sType
        nullptr,                                                    // pNext
        0,                                                          // flags
        VK_FALSE,                                                   // logicOpEnable
        VK_LOGIC_OP_COPY,                                           // logicOp
        1,                                                          // attachmentCount
        &colorBlendAttachmentState,                                 // pAttachments
        { 0.0f, 0.0f, 0.0f, 0.0f }                                  // blendConstants[4]
    };

    constexpr VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,   // sType
        nullptr,                                                    // pNext
        0,                                                          // flags
        VK_SAMPLE_COUNT_1_BIT,                                      // rasterizationSamples
        VK_FALSE,                                                   // sampleShadingEnable
        1.0f,                                                       // minSampleShading
        nullptr,                                                    // pSampleMask
        VK_FALSE,                                                   // alphaToCoverageEnable
        VK_FALSE,                                                   // alphaToOneEnable
    };

    constexpr VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
    {
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,  // sType
        nullptr,                                        // pNext
        0,                                              // flags
        0,                                              // setLayoutCount
        nullptr,                                        // pSetLayouts
        0,                                              // pushConstantRangeCount
        nullptr                                         // pPushConstantRanges
    };

    CHECK_VK_RESULT_SUCCESS(vkCreatePipelineLayout(
        m_device,                       // device
        &pipelineLayoutCreateInfo,      // pCreateInfo
        nullptr,                        // pAllocator,
        &m_pipelineLayout));            // pPipelineLayout

    const VkGraphicsPipelineCreateInfo pipelineCreateInfo =
    {
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,    // sType
        nullptr,                        // pNext
        0,                              // flags
        2,                              // stageCount
        &shaderStageCreateInfo[0],      // pStages
        &vertexInputStateCreateInfo,    // pVertexInputState
        &inputAssemblyStateCreateInfo,  // pInputAssemblyState
        nullptr,                        // pTessellationState
        &viewportStateCreateInfo,       // pViewportState
        &rasterizationStateCreateInfo,  // pRasterizationState
        &multisampleStateCreateInfo,    // pMultisampleState
        nullptr,                        // pDepthStencilState
        &colorBlendStateCreateInfo,     // pColorBlendState
        nullptr,                        // pDynamicState
        m_pipelineLayout,               // layout
        m_renderPass,                   // renderPass
        0,                              // subpass
        nullptr,                        // basePipelineHandle
        0                               // basePipelineIndex
    };

    CHECK_VK_RESULT_SUCCESS(vkCreateGraphicsPipelines(
        m_device,               // device
        nullptr,                // pipelineCache
        1,                      // createInfoCount
        &pipelineCreateInfo,    // pCreateInfos
        nullptr,                // pAllocator
        &m_graphicsPipeline));  // pPipelines
}

void GfxResources::createQueueAndPool()
{
    vkGetDeviceQueue(
        m_device,           // device
        m_queueFamilyIndex, // queueFamilyIndex
        0,                  // queueIndex
        &m_queue);          // pQueue
    assert(m_queue);

    const VkCommandPoolCreateInfo commandPoolCreateInfo =
    {
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,         // sType
        nullptr,                                            // pNext
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,    // flags
        m_queueFamilyIndex                                  // queueFamilyIndex
    };

    CHECK_VK_RESULT_SUCCESS(vkCreateCommandPool(
        m_device,               // device
        &commandPoolCreateInfo, // pCreateInfo
        nullptr,                // pAllocator
        &m_commandPool));       // pCommandPool
    assert(m_commandPool);

    // create command buffers and their fences
    m_bufferedFrameResource.commandBuffers.resize(m_bufferedFrameResource.bufferCount);
    m_bufferedFrameResource.commandBufferFences.resize(m_bufferedFrameResource.bufferCount);

    const VkCommandBufferAllocateInfo commandBufferAllocateInfo =
    {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // sType
        nullptr,                                        // pNext
        m_commandPool,                                  // commandPool
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,                // level
        m_bufferedFrameResource.bufferCount             // commandBufferCount
    };

    CHECK_VK_RESULT_SUCCESS(vkAllocateCommandBuffers(
        m_device,                                       // device
        &commandBufferAllocateInfo,                     // pAllocateInfo
        m_bufferedFrameResource.commandBuffers.data()));// pCommandBuffers

    // set as signaled, so we pass the vkWaitForFences() the first time
    constexpr VkFenceCreateInfo fenceCreateInfo =
    {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,    // sType
        nullptr,                                // pNext
        VK_FENCE_CREATE_SIGNALED_BIT            // flags
    };

    for (uint32_t idx = 0; idx < m_bufferedFrameResource.bufferCount; ++idx)
    {
        CHECK_VK_RESULT_SUCCESS(vkCreateFence(
            m_device,                                           // device
            &fenceCreateInfo,                                   // pCreateInfo
            nullptr,                                            // pAllocator
            &m_bufferedFrameResource.commandBufferFences[idx]));// pFence
    }
}

void GfxResources::createSemaphores()
{
    constexpr VkSemaphoreCreateInfo semaphoreCreateInfo =
    {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,    // sType
        nullptr,                                    // pNext
        0,                                          // flags
    };

    CHECK_VK_RESULT_SUCCESS(vkCreateSemaphore(
        m_device,                                           // device
        &semaphoreCreateInfo,                               // pCreateInfo
        nullptr,                                            // pAllocator
        &m_bufferedFrameResource.swapchainImageSemaphore)); // pSemaphore

    CHECK_VK_RESULT_SUCCESS(vkCreateSemaphore(
        m_device,                                           // device
        &semaphoreCreateInfo,                               // pCreateInfo
        nullptr,                                            // pAllocator
        &m_bufferedFrameResource.cmdBufferSubmitSemaphore));// pSemaphore
}

GfxResources::BufferedFrameResource& GfxResources::getBufferedFrameResource()
{
    return m_bufferedFrameResource;
}

VkDevice GfxResources::getDevice()
{
    return m_device;
}

VkSwapchainKHR GfxResources::getSwapchain()
{
    return m_swapchain;
}

VkRenderPass GfxResources::getRenderPass()
{
    return m_renderPass;
}

VkPipeline GfxResources::getGraphicsPipeline()
{
    return m_graphicsPipeline;
}

VkQueue GfxResources::getQueue()
{
    return m_queue;
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
