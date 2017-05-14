#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H

// Copyright (c) 2017 Johannes Pystynen
// This code is licensed under the MIT license (MIT)

#include <memory>

///////////////////////////////////////////////////////////////////////////////

namespace core
{

class GfxResources;
class Renderer;
class Window;

class Engine
{
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    void init();
    void run();

private:
    std::unique_ptr<GfxResources> m_gfxResources;

    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<Window> m_window;
};

} // namespace

///////////////////////////////////////////////////////////////////////////////

#endif // CORE_ENGINE_H
