#include "platform/win32/TrayIcon.hpp"
#include <cwchar>
#include <filesystem>
namespace winpod
{
TrayIcon::TrayIcon() {}
TrayIcon::~TrayIcon()
{
    Destroy();
    if (m_hIcon)
    {
        DestroyIcon(m_hIcon);
        m_hIcon = nullptr;
    }
}
bool TrayIcon::Create(HWND hwnd, const std::string& name)
{
    Destroy();
    m_Hwnd = hwnd;
    m_TrayIconName = winrt::to_hstring(name);
    m_TrayIcon = {};
    m_TrayIcon.cbSize = sizeof(m_TrayIcon);
    m_TrayIcon.hWnd = hwnd;
    m_TrayIcon.uID = kTrayIconId;
    m_TrayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_TrayIcon.uCallbackMessage = WM_TRAYICON;
    m_TrayIcon.hIcon = m_hIcon ? m_hIcon : LoadIcon(nullptr, IDI_APPLICATION);
    wcsncpy_s(m_TrayIcon.szTip, m_TrayIconName.c_str(), _TRUNCATE);
    m_bCreated = Shell_NotifyIconW(NIM_ADD, &m_TrayIcon) != FALSE;
    return m_bCreated;
}
bool TrayIcon::LoadICO(const std::filesystem::path& ico_path)
{
    if (!std::filesystem::exists(ico_path))
    {
        return false;
    }
    const UINT dpi = m_Hwnd ? GetDpiForWindow(m_Hwnd) : GetDpiForSystem();
    HICON icon =
        static_cast<HICON>(LoadImageW(nullptr, ico_path.c_str(), IMAGE_ICON, GetSystemMetricsForDpi(SM_CXSMICON, dpi),
                                      GetSystemMetricsForDpi(SM_CYSMICON, dpi), LR_LOADFROMFILE));
    if (!icon)
    {
        return false;
    }
    m_TrayIcon.hIcon = icon;
    if (m_bCreated)
    {
        m_TrayIcon.uFlags = NIF_ICON;
        Shell_NotifyIconW(NIM_MODIFY, &m_TrayIcon);
        m_TrayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    }
    if (m_hIcon)
    {
        DestroyIcon(m_hIcon);
    }
    m_hIcon = icon;
    return true;
}
void TrayIcon::ShowMenu(HWND hwnd)
{
    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_SHOW, L"Open");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    POINT pt{};
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
    DestroyMenu(hMenu);
}
void TrayIcon::Destroy()
{
    if (!m_bCreated)
    {
        return;
    }
    Shell_NotifyIconW(NIM_DELETE, &m_TrayIcon);
    m_bCreated = false;
}
} // namespace winpod
