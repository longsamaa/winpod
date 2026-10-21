#pragma once

#ifndef _D3D11_RENDERER_HPP_
#define _D3D11_RENDERER_HPP_
#include "render/IRenderer.hpp"
#include "platform/WindowsSdk.hpp"
#include <unordered_map>
namespace winpod
{
class D3D11Renderer final : public IRenderer
{
public:
    D3D11Renderer() = default;
    ~D3D11Renderer() override;
    D3D11Renderer(const D3D11Renderer&) = delete;
    D3D11Renderer& operator=(const D3D11Renderer&) = delete;

    bool Create(HWND hwnd) override;
    void Destroy() override;
    void Resize(UINT width, UINT height) override;
    void BeginFrame(const float clearColor[4]) override;
    void EndFrame() override;

    bool InitImGui() override;
    void ShutdownImGui() override;
    void ImGuiNewFrame() override;
    void ImGuiRender(ImDrawData* drawData) override;

    ImTextureID CreateTexture(UINT width, UINT height) override;
    void UpdateTexture(ImTextureID texture, const void* rgba, UINT pitch) override;
    void DestroyTexture(ImTextureID texture) override;

    void BeginSmoothSampling(ImDrawList* drawList) override;
    void EndSmoothSampling(ImDrawList* drawList) override;

private:
    bool CreateRT();
    void CleanupRT();
    static void SetMipSamplerCallback(const ImDrawList* parentList, const ImDrawCmd* cmd);

    struct Texture
    {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        bool mipmapped{false};
    };

private:
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_MipSampler;
    std::unordered_map<ImTextureID, Texture> m_Textures;
    Microsoft::WRL::ComPtr<ID3D11Device> m_Device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_Context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_SwapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_Rtv;
    bool m_ImGuiInitialized{false};
};
} // namespace winpod
#endif //_D3D11_RENDERER_HPP_
