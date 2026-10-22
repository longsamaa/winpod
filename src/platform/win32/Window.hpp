#pragma once
#ifndef _WINDPOD_WINDOW_HPP_
#define _WINDPOD_WINDOW_HPP_
#include "platform/WindowsSdk.hpp"
#include "platform/win32/TrayIcon.hpp"
#include <string>
#include <functional>
namespace winpod
{
class Window
{
public:
    using MessageHook = std::function<bool(HWND, UINT, WPARAM, LPARAM)>;

    Window();
    ~Window();

public:
    bool Create(const std::string& name, HINSTANCE& hInst);
    void PollMessage();
    void SetMessageHook(MessageHook hook);
    HWND GetHwnd() const;
    bool Shutdown() const;
    bool ConsumeResize(UINT& width, UINT& height);
    void SetClientWidth(int width);
    void Hide();
    void RequestHide();
    bool HidePending() const;
    void Show();
    bool IsShow() const;
    void registerWindowHideCallback(const std::function<void()>& callback);

private:
    static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);
    void PlaceBottomRight(HWND hwnd);
    void Destroy();

private:
    std::string config_path{"config//config.json"};
    MessageHook m_MessageHook{nullptr};
    HWND m_Hwnd{nullptr};
    HINSTANCE m_HInst{nullptr};
    UINT m_ResizeW{0};
    UINT m_ResizeH{0};
    int m_ClientW{0};
    bool m_bIsShutdown{false};
    TrayIcon m_TrayIcon{};
    std::wstring m_ClassName;
    WNDCLASSEXW m_Wc;
    // Callback
    std::function<void()> onWindowHide{nullptr};
    bool m_bHidePending{false};
};
} // namespace winpod
#endif
