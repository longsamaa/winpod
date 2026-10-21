#pragma once
#ifndef _WINPOD_FAKE_RENDERER_HPP_
#define _WINPOD_FAKE_RENDERER_HPP_
#include "render/IRenderer.hpp"
#include <set>

namespace winpod::test
{
// IRenderer không cần GPU: chỉ ghi lại texture nào đang sống để test kiểm tra tạo/huỷ.
class FakeRenderer final : public IRenderer
{
public:
    bool Create(HWND) override { return true; }
    void Destroy() override {}
    void Resize(UINT, UINT) override {}
    void BeginFrame(const float[4]) override {}
    void EndFrame() override {}
    bool InitImGui() override { return true; }
    void ShutdownImGui() override {}
    void ImGuiNewFrame() override {}
    void ImGuiRender(ImDrawData*) override {}

    ImTextureID CreateTexture(UINT width, UINT height) override
    {
        ++createCount;
        lastWidth = width;
        lastHeight = height;
        if (failCreate)
        {
            return ImTextureID_Invalid;
        }
        const ImTextureID id = nextId++;
        live.insert(id);
        return id;
    }

    void UpdateTexture(ImTextureID texture, const void* rgba, UINT pitch) override
    {
        ++updateCount;
        lastUpdated = texture;
        lastPitch = pitch;
        lastPixels = rgba;
    }

    void DestroyTexture(ImTextureID texture) override
    {
        ++destroyCount;
        live.erase(texture);
    }

    void BeginSmoothSampling(ImDrawList*) override {}
    void EndSmoothSampling(ImDrawList*) override {}

    // Cấu hình
    bool failCreate{false};

    // Quan sát
    std::set<ImTextureID> live;
    int createCount{0};
    int updateCount{0};
    int destroyCount{0};
    UINT lastWidth{0};
    UINT lastHeight{0};
    UINT lastPitch{0};
    ImTextureID lastUpdated{ImTextureID_Invalid};
    const void* lastPixels{nullptr};

private:
    ImTextureID nextId{1};
};
} // namespace winpod::test
#endif // _WINPOD_FAKE_RENDERER_HPP_
