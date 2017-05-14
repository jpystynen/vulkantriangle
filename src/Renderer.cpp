// Copyright (c) 2017 Johannes Pystynen
// This code is licensed under the MIT license (MIT)

#include "Renderer.h"

#include "GfxResources.h"
#include "Window.h"

#include <memory>
#include <string>
#include <assert.h>
#include <iostream>

#include <vulkan/vulkan.h>

///////////////////////////////////////////////////////////////////////////////

namespace core
{

Renderer::Renderer(GfxResources* const p_gfxResources, Window* const p_window)
    : mp_gfxResources(p_gfxResources),
    mp_window(p_window)
{
    assert(mp_gfxResources);
    assert(mp_window);
}

void Renderer::render()
{
    // not using pre-recorded command buffers
    // does the same setup every frame
    // for buffered resources

    VkDevice device = mp_gfxResources->getDevice();
    VkSwapchainKHR swapchain = mp_gfxResources->getSwapchain();
    VkRenderPass renderPass = mp_gfxResources->getRenderPass();
    VkPipeline graphicsPipeline = mp_gfxResources->getGraphicsPipeline();
    VkQueue queue = mp_gfxResources->getQueue();

    GfxResources::BufferedFrameResource& frameResource =
        mp_gfxResources->getBufferedFrameResource();

    VkSemaphore swapchainImageSemaphore = frameResource.swapchainImageSemaphore;

    // get index for buffered resources
    {
        CHECK_VK_RESULT_SUCCESS(vkAcquireNextImageKHR(
            device,                         // device
            swapchain,                      // swapchin
            0,                              // timeout
            swapchainImageSemaphore,        // semaphore
            nullptr,                        // fence
            &frameResource.bufferIndex));   //  pImageIndex
        assert(frameResource.bufferIndex < (uint32_t)frameResource.images.size());
    }

    const uint32_t currIndex = frameResource.bufferIndex;

    VkFramebuffer framebuffer = frameResource.framebuffers[currIndex];
    VkCommandBuffer cmdBuffer = frameResource.commandBuffers[currIndex];
    VkFence cmdBufferFence = frameResource.commandBufferFences[currIndex];
    VkSemaphore cmdBufferSubmitSemaphore = frameResource.cmdBufferSubmitSemaphore;

    // setup command buffer
    {
        const uint32_t timeout = s_defaultTimeout;
        CHECK_VK_RESULT_SUCCESS(vkWaitForFences(
            device,             // device
            1,                  // fenceCount
            &cmdBufferFence,    // pFences
            VK_TRUE,            // waitAll
            timeout));          // timeout

        CHECK_VK_RESULT_SUCCESS(vkResetCommandBuffer(
            cmdBuffer,  // commandBuffer
            0));        // flags

        constexpr VkCommandBufferBeginInfo commandBufferBeginInfo =
        {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,    // sType
            nullptr,                                        // pNext
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,    // flags
            nullptr                                         // pInheritanceInfo
        };

        CHECK_VK_RESULT_SUCCESS(vkBeginCommandBuffer(
            cmdBuffer,                  // commandBuffer
            &commandBufferBeginInfo));  // pBeginInfo
    }

    const uint32_t width = mp_window->getWidth();
    const uint32_t height = mp_window->getHeight();

    const VkRect2D renderArea =
    {
        { 0, 0 },           // offset
        { width, height}    // extent
    };
    constexpr VkClearValue clearValue =
    {
        { 0.0f, 0.0f, 0.0f, 0.0f }, // color
    };

    const VkRenderPassBeginInfo renderPassBeginInfo =
    {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,   // sType
        nullptr,                                    // pNext
        renderPass,                                 // renderPass
        framebuffer,                                // framebuffer
        renderArea,                                 // renderArea
        1,                                          // clearValueCount
        &clearValue                                 // pClearValues;
    };

    vkCmdBeginRenderPass(
        cmdBuffer,                      // commandBuffer
        &renderPassBeginInfo,           // pRenderPassBegin
        VK_SUBPASS_CONTENTS_INLINE);    // contents

    vkCmdBindPipeline(
        cmdBuffer,                          // commandBuffer
        VK_PIPELINE_BIND_POINT_GRAPHICS,    // pipelineBindPoint
        graphicsPipeline);                  // pipeline

    vkCmdDraw(
        cmdBuffer,  // commandBuffer
        3,          // vertexCount
        1,          // instanceCount
        0,          // firstVertex
        0);         // firstInstance

    vkCmdEndRenderPass(cmdBuffer);

    CHECK_VK_RESULT_SUCCESS(vkEndCommandBuffer(cmdBuffer));

    // submit
    {
        vkResetFences(
            device,             // device
            1,                  // fenceCount
            &cmdBufferFence);   // pFences

        constexpr VkPipelineStageFlags waitStageFlags =
        {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };

        const VkSubmitInfo submitInfo =
        {
            VK_STRUCTURE_TYPE_SUBMIT_INFO,  // sType
            nullptr,                        // pNext
            1,                              // waitSemaphoreCount
            &swapchainImageSemaphore,       // pWaitSemaphores
            &waitStageFlags,                // pWaitDstStageMask
            1,                              // commandBufferCount
            &cmdBuffer,                     // pCommandBuffers
            1,                              // signalSemaphoreCount
            &cmdBufferSubmitSemaphore       // pSignalSemaphores
        };

        CHECK_VK_RESULT_SUCCESS(vkQueueSubmit(
            queue,              // queue
            1,                  // submitCount
            &submitInfo,        // pSubmits
            cmdBufferFence));   // fence
    }

    // present
    {
        const VkPresentInfoKHR presentInfo =
        {
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, // sType
            nullptr,                            // pNext
            1,                                  // waitSemaphoreCount
            &cmdBufferSubmitSemaphore,          // pWaitSemaphores
            1,                                  // swapchainCount
            &swapchain,                         // pSwapchains
            &currIndex,                         // pImageIndices
            nullptr                             // pResults
        };

        CHECK_VK_RESULT_SUCCESS(vkQueuePresentKHR(
            queue,          // queue
            &presentInfo)); // pPresentInfo
    }
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
