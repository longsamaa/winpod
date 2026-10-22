#pragma once
#include "imgui.h"
#ifndef _IM_GUI_LAYER_HPP_
#define _IM_GUI_LAYER_HPP_
#include "platform/WindowsSdk.hpp"
#include <imgui.h>
namespace winpod
{
class Window;

class ImGuiLayer
{
public:
    ImGuiLayer();
    ~ImGuiLayer();

public:
    bool Init(Window& window);
    void NewFrame();
    ImDrawData* Render();
    void Shutdown();
    static ImFont* SemiBoldFont();

private:
    static ImFont* s_SemiBoldFont;
    ImGuiContext* m_Context{nullptr};
    Window* m_Window{nullptr};
};
} // namespace winpod
#endif //_IM_GUI_LAYER_HPP_
