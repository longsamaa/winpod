#pragma once
#ifndef _IRENDERER_HPP_
#define _IRENDERER_HPP_
#include "platform/WindowsSdk.hpp"
#include <imgui.h> // ImDrawData, ImTextureID

namespace winpod
{
class IRenderer
{
public:
    virtual ~IRenderer() = default;
    virtual bool Create(HWND hwnd) = 0;
    virtual void Destroy() = 0;
    virtual void Resize(UINT width, UINT height) = 0;
    virtual void BeginFrame(const float clearColor[4]) = 0;
    virtual void EndFrame() = 0; // Present
    virtual bool InitImGui() = 0;
    virtual void ShutdownImGui() = 0;
    virtual void ImGuiNewFrame() = 0;
    virtual void ImGuiRender(ImDrawData* drawData) = 0;
    virtual ImTextureID CreateTexture(UINT width, UINT height) = 0;
    virtual void UpdateTexture(ImTextureID texture, const void* rgba, UINT pitch) = 0;
    virtual void DestroyTexture(ImTextureID texture) = 0;
    virtual void BeginSmoothSampling(ImDrawList* drawList) = 0;
    virtual void EndSmoothSampling(ImDrawList* drawList) = 0;
};
} // namespace winpod
#endif //_IRENDERER_HPP_
