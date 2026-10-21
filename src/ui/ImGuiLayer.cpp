#include "ImGuiLayer.hpp"
#include "app/Window.hpp"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "log/Log.hpp"
#include "ui/Font.hpp"
#include <filesystem>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace winpod
{

ImFont* ImGuiLayer::s_SemiBoldFont = nullptr;

ImFont* ImGuiLayer::SemiBoldFont()
{
    return s_SemiBoldFont;
}

ImGuiLayer::ImGuiLayer() = default;
ImGuiLayer::~ImGuiLayer()
{
    Shutdown();
}
bool ImGuiLayer::Init(Window& window)
{
    HWND hwnd = window.GetHwnd();
    m_Context = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    float s = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
    auto logger = Log::instance().getAppLogger();
    if (!Font::LoadFont(kRegularFontPath, 18 * s))
    {
        logger->warn("[IMGUI] Missing font {}, using ImGui default font", kRegularFontPath);
        io.Fonts->AddFontDefault();
    }
    Font::Merge(kIconFontPath, 18 * s);
    s_SemiBoldFont = Font::LoadFont(kSemiBoldFontPath, 18 * s);
    if (s_SemiBoldFont)
    {
        Font::Merge(kIconFontPath, 18 * s);
    }
    else
    {
        logger->warn("[IMGUI] Missing font {}", kSemiBoldFontPath);
    }
    if (!std::filesystem::exists(kIconFontPath))
    {
        logger->warn("[IMGUI] Missing icon font {}", kIconFontPath);
    }
    ImGui::GetStyle().ScaleAllSizes(s);
    if (!ImGui_ImplWin32_Init(hwnd))
    {
        logger->error("[IMGUI] Create Imgui impl win32 fail");
        return false;
    }
    m_Window = &window;
    m_Window->SetMessageHook(
        [](HWND h, UINT msg, WPARAM wp, LPARAM lp) { return ImGui_ImplWin32_WndProcHandler(h, msg, wp, lp) != 0; });
    logger->trace("[IMGUI LAYER] Create ImGui Layer");
    return true;
}
void ImGuiLayer::NewFrame()
{
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}
ImDrawData* ImGuiLayer::Render()
{
    ImGui::Render();
    return ImGui::GetDrawData();
}
void ImGuiLayer::Shutdown()
{
    if (m_Window)
    {
        m_Window->SetMessageHook(nullptr);
        m_Window = nullptr;
    }
    if (m_Context)
    {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(m_Context);
        m_Context = nullptr;
        s_SemiBoldFont = nullptr;
    }
}
} // namespace winpod
