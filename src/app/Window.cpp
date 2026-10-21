#include "Window.hpp"
#include <log/Log.hpp>
#include <algorithm>
#include <cstdlib>
#include <iostream>
namespace winpod
{
namespace
{
constexpr int FORM_W = 250, FORM_H = 250;
constexpr int MARGIN = 12;
constexpr int SH = 24;
constexpr int PADDING = 10;
} // namespace

Window::Window() {};
Window::~Window()
{
    Destroy();
}
LRESULT WINAPI Window::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Window* self = nullptr;
    if (msg == WM_NCCREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<Window*>(cs->lpCreateParams);
        self->m_Hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else
    {
        self = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (self)
    {
        return self->HandleMessage(msg, wp, lp);
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void Window::SetMessageHook(MessageHook hook)
{
    m_MessageHook = std::move(hook);
}

HWND Window::GetHwnd() const
{
    return m_Hwnd;
}

LRESULT Window::HandleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    if (m_MessageHook && m_MessageHook(m_Hwnd, msg, wp, lp))
    {
        return TRUE;
    }
    switch (msg)
    {
    case WM_CREATE: {
        if (!m_TrayIcon.LoadICO("resources/icons/winpod.ico"))
        {
            Log::instance().getAppLogger()->error("[WINDOW] Can't load tray .ico");
        }
        if (!m_TrayIcon.Create(m_Hwnd, "WinPod"))
        {
            Log::instance().getAppLogger()->error("[WINDOW] Can't add tray icon");
        }
        return 0;
    }
    case WM_TRAYICON: {
        if (lp == WM_RBUTTONUP)
        {
            m_TrayIcon.ShowMenu(m_Hwnd);
        }
        if (lp == WM_LBUTTONDOWN)
        {
            if (!IsShow())
            {
                Show();
            }
        }
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wp))
        {
        case ID_TRAY_SHOW: {
            if (!IsShow())
            {
                Show();
            }
        }
            return 0;
        case ID_TRAY_EXIT:
            DestroyWindow(m_Hwnd);
            return 0;
        }
        break;
    case WM_SIZE:
        if (wp != SIZE_MINIMIZED)
        {
            m_ResizeW = LOWORD(lp);
            m_ResizeH = HIWORD(lp);
        }
        return 0;
    case WM_DISPLAYCHANGE:
    case WM_SETTINGCHANGE:
    case WM_DPICHANGED:
        PlaceBottomRight(m_Hwnd);
        return 0;
    case WM_NCDESTROY: {
        HWND hwnd = m_Hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        m_Hwnd = nullptr;
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
    case WM_DESTROY:
        m_TrayIcon.Destroy();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(m_Hwnd, msg, wp, lp);
}
void Window::PollMessage()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
        {
            m_bIsShutdown = true;
        }
    }
}
bool Window::Shutdown() const
{
    return m_bIsShutdown;
}
bool Window::ConsumeResize(UINT& width, UINT& height)
{
    if (m_ResizeW == 0 || m_ResizeH == 0)
    {
        return false;
    }
    width = m_ResizeW;
    height = m_ResizeH;
    m_ResizeW = m_ResizeH = 0;
    return true;
}
void Window::PlaceBottomRight(HWND hwnd)
{
    MONITORINFO mi = {sizeof(mi)};
    GetMonitorInfoW(MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY), &mi);
    RECT wa = mi.rcWork;
    float s = static_cast<float>(GetDpiForWindow(hwnd)) / USER_DEFAULT_SCREEN_DPI;
    int w = m_ClientW > 0 ? m_ClientW : static_cast<int>(FORM_W * s);
    int h = static_cast<int>(FORM_H * s);
    int x = wa.right - w - PADDING;
    int y = wa.bottom - h - PADDING;
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);
}

void Window::SetClientWidth(int width)
{
    if (!m_Hwnd || width <= 0)
        return;
    MONITORINFO mi = {sizeof(mi)};
    GetMonitorInfoW(MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY), &mi);
    const float s = static_cast<float>(GetDpiForWindow(m_Hwnd)) / USER_DEFAULT_SCREEN_DPI;
    const int minW = static_cast<int>(FORM_W * s);
    const int maxW = std::max(minW, static_cast<int>((mi.rcWork.right - mi.rcWork.left) * 0.4f));
    const int clamped = std::clamp(width, minW, maxW);

    const int current = m_ClientW > 0 ? m_ClientW : minW;
    if (std::abs(clamped - current) <= 1)
        return;
    m_ClientW = clamped;
    PlaceBottomRight(m_Hwnd);
}

void Window::RequestHide()
{
    m_bHidePending = true;
}

bool Window::HidePending() const
{
    return m_bHidePending;
}

void Window::Hide()
{
    m_bHidePending = false;
    if (m_Hwnd)
    {
        ShowWindow(m_Hwnd, SW_HIDE);
        if (onWindowHide)
            onWindowHide();
    }
}

void Window::Show()
{
    m_bHidePending = false;
    if (m_Hwnd)
    {
        PlaceBottomRight(m_Hwnd);
        ShowWindow(m_Hwnd, SW_SHOWNOACTIVATE);
    }
}

bool Window::IsShow() const
{
    if (!m_Hwnd)
        return false;
    return IsWindowVisible(m_Hwnd);
}

void Window::registerWindowHideCallback(const std::function<void()>& callback)
{
    onWindowHide = std::move(callback);
}

bool Window::Create(const std::string& name, HINSTANCE& hInst)
{
    m_ClassName = winrt::to_hstring(name);
    m_Wc = {sizeof(m_Wc), CS_CLASSDC,          WndProc, 0, 0, hInst, nullptr, LoadCursor(nullptr, IDC_ARROW), nullptr,
            nullptr,      m_ClassName.c_str(), nullptr};
    if (!RegisterClassExW(&m_Wc))
    {
        return false;
    }
    m_HInst = hInst;
    CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, m_Wc.lpszClassName, L"", WS_POPUP, 0, 0, 100, 100,
                    nullptr, nullptr, hInst, this);
    auto logger = Log::instance().getAppLogger();
    if (!m_Hwnd)
    {
        logger->error("[WINDOW] Can't create HWND {}", name);
        return false;
    }
    logger->trace("[WINDOW] Create Window {}", name);
    SetLayeredWindowAttributes(m_Hwnd, 0, 255, LWA_ALPHA);
    MARGINS mg = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(m_Hwnd, &mg);
    const BOOL disableTransitions = TRUE;
    DwmSetWindowAttribute(m_Hwnd, DWMWA_TRANSITIONS_FORCEDISABLED, &disableTransitions, sizeof(disableTransitions));
    PlaceBottomRight(m_Hwnd);
    Show();
    return true;
}
void Window::Destroy()
{
    if (m_Hwnd)
    {
        DestroyWindow(m_Hwnd);
    }
    if (m_HInst)
    {
        UnregisterClassW(m_Wc.lpszClassName, m_HInst);
        m_HInst = nullptr;
    }
}
} // namespace winpod
