#ifndef CORE_RENDERER_H
#define CORE_RENDERER_H

// Copyright (c) 2017 Johannes Pystynen
// This code is licensed under the MIT license (MIT)

#include <memory>

#include <vulkan/vulkan.h>

///////////////////////////////////////////////////////////////////////////////

namespace core
{

class GfxDevice;
class GfxResources;
class Window;

class Renderer
{
public:
    Renderer(GfxResources* const p_gfxResources, Window* const p_window);
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void render();

private:
    GfxResources* const mp_gfxResources = nullptr;
    Window* const mp_window             = nullptr;
};

} // namespace

///////////////////////////////////////////////////////////////////////////////

#endif // CORE_RENDERER_H
