#ifndef CORE_WINDOW_H
#define CORE_WINDOW_H

// Copyright (c) 2017 Johannes Pystynen
// This code is licensed under the MIT license (MIT)

#include <string>

#include <windows.h>

///////////////////////////////////////////////////////////////////////////////

namespace core
{

class Window
{
public:
    Window(const uint32_t width, const uint32_t height, const std::string& name);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void update();
    bool shouldClose() const;

    uint32_t getWidth() const;
    uint32_t getHeight() const;

    HWND getHwnd() const;
    HINSTANCE getHinstance() const;

private:

    HWND m_hwnd;
    HINSTANCE m_hinstance;

    uint32_t m_width    = 0;
    uint32_t m_height   = 0;

    std::string m_name;
    bool m_closeWindow = false;
};

} // namespace

///////////////////////////////////////////////////////////////////////////////

#endif // CORE_WINDOW_H
