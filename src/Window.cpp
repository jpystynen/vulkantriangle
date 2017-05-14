// Copyright (c) 2017 Johannes Pystynen
// This code is licensed under the MIT license (MIT)

#include "Window.h"

#include <string>
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////

namespace core
{

    static LRESULT windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        } break;
        default:break;
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    Window::Window(const uint32_t width, const uint32_t height, const std::string& name)
        : m_width(width), m_height(height), m_name(name)
    {
        m_hinstance = GetModuleHandle(nullptr);

        WNDCLASSEX wcex;
        ZeroMemory(&wcex, sizeof(WNDCLASSEX));

        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = windowProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = m_hinstance;
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = m_name.c_str();
        //wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
        const auto res = RegisterClassEx(&wcex);
        assert(res > 0);

        DWORD dwStyle = WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        RECT winRect = { 0, 0, LONG(width), LONG(height) };

        const BOOL success = AdjustWindowRectEx(&winRect, dwStyle, FALSE, dwExStyle);
        assert(success);

        m_hwnd = CreateWindowEx(
            0,
            m_name.c_str(),
            m_name.c_str(),
            dwStyle,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            winRect.right - winRect.left,
            winRect.bottom - winRect.top,
            nullptr,
            nullptr,
            m_hinstance,
            nullptr);
        assert(m_hwnd);

        ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    }

Window::~Window()
{
    DestroyWindow(m_hwnd);
    UnregisterClass(m_name.c_str(), m_hinstance);
}

void Window::update()
{
    MSG msg;
    if (PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
        {
            m_closeWindow = true;
        }
    }
}

bool Window::shouldClose() const
{
    return m_closeWindow;
}

uint32_t Window::getWidth() const
{
    return m_width;
}

uint32_t Window::getHeight() const
{
    return m_height;
}

HWND Window::getHwnd() const
{
    return m_hwnd;
}

HINSTANCE Window::getHinstance() const
{
    return m_hinstance;
}

} // namespace

///////////////////////////////////////////////////////////////////////////////
