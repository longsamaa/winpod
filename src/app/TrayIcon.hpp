#pragma once
#include <string>
#ifndef _WINPOD_TRAY_ICON_HPP_
#define _WINPOD_TRAY_ICON_HPP_
#include "platform/WindowsSdk.hpp"
#include <filesystem>
namespace winpod
{
static constexpr UINT WM_TRAYICON = WM_APP + 1;
static constexpr UINT kTrayIconId = 1;
constexpr UINT ID_TRAY_SHOW = 1001;
constexpr UINT ID_TRAY_HIDE = 1002;
constexpr UINT ID_TRAY_EXIT = 1003;
class TrayIcon
{
public:
    TrayIcon();
    ~TrayIcon();
    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;

public:
    bool Create(HWND hwnd, const std::string& name);
    void Destroy();
    void ShowMenu(HWND hwnd);
    bool LoadICO(const std::filesystem::path& ico_path);

private:
    HWND m_Hwnd{nullptr};
    NOTIFYICONDATAW m_TrayIcon{};
    std::wstring m_TrayIconName{};
    bool m_bCreated{false};
    HICON m_hIcon{nullptr};
};
} // namespace winpod
#endif
