// Copyright (c) 2017 Johannes Pystynen
// This code is licensed under the MIT license (MIT)

#include "Engine.h"

#include "GfxResources.h"
#include "Renderer.h"
#include "Window.h"
#include "Utils.h"

#include <memory>

///////////////////////////////////////////////////////////////////////////////

namespace core
{

Engine::Engine()
{

}

Engine::~Engine()
{

}

void Engine::init()
{
    GlobalVariables& gv = GlobalVariables::getInstance();
    gv.applicationName = "Vulkan Triangle";
    gv.engineName = "Dummy Engine";

    m_window = std::unique_ptr<Window>(new Window(
        gv.windowWidth, gv.windowHeight, gv.applicationName));

    m_gfxResources = std::unique_ptr<GfxResources>(new GfxResources(m_window.get()));
    m_renderer = std::unique_ptr<Renderer>(new Renderer(
        m_gfxResources.get(), m_window.get()));
}

void Engine::run()
{
    while (!m_window->shouldClose())
    {
        m_window->update();
        m_renderer->render();
    }
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
