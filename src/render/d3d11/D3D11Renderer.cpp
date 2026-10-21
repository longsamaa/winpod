#include "D3D11Renderer.hpp"
#include "log/Log.hpp"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <algorithm>
#include <cstring>
#include <iterator>

namespace winpod
{
D3D11Renderer::~D3D11Renderer()
{
    ShutdownImGui();
    Destroy();
}

bool D3D11Renderer::Create(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels,
                                               static_cast<UINT>(std::size(levels)), D3D11_SDK_VERSION, &sd,
                                               &m_SwapChain, &m_Device, &fl, &m_Context);
    auto logger = Log::instance().getAppLogger();
    if (FAILED(hr))
    {
        logger->error("[D3D11] Create device failed: 0x{:08X}", static_cast<uint32_t>(hr));
        return false;
    }
    if (!CreateRT())
    {
        Destroy();
        return false;
    }
    D3D11_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = 3.0f;
    hr = m_Device->CreateSamplerState(&sampler, &m_MipSampler);
    if (FAILED(hr))
    {
        logger->warn("[D3D11] Create mip sampler failed: 0x{:08X}", static_cast<uint32_t>(hr));
    }
    logger->trace("[RENDERER] Create D3D11 Renderer");
    return true;
}

bool D3D11Renderer::CreateRT()
{
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    HRESULT hr = m_SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (SUCCEEDED(hr))
    {
        hr = m_Device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_Rtv);
    }
    if (FAILED(hr))
    {
        Log::instance().getAppLogger()->error("[D3D11] Create render target failed: 0x{:08X}",
                                              static_cast<uint32_t>(hr));
        return false;
    }
    return true;
}

void D3D11Renderer::CleanupRT()
{
    m_Rtv.Reset();
}

void D3D11Renderer::Destroy()
{
    m_Textures.clear();
    m_MipSampler.Reset();
    CleanupRT();
    m_SwapChain.Reset();
    m_Context.Reset();
    m_Device.Reset();
}

void D3D11Renderer::Resize(UINT width, UINT height)
{
    if (!m_SwapChain || width == 0 || height == 0)
    {
        return;
    }
    CleanupRT();
    HRESULT hr = m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        Log::instance().getAppLogger()->error("[D3D11] ResizeBuffers failed: 0x{:08X}", static_cast<uint32_t>(hr));
    }
    CreateRT();
}

void D3D11Renderer::BeginFrame(const float clearColor[4])
{
    m_Context->OMSetRenderTargets(1, m_Rtv.GetAddressOf(), nullptr);
    m_Context->ClearRenderTargetView(m_Rtv.Get(), clearColor);
}

void D3D11Renderer::EndFrame()
{
    m_SwapChain->Present(1, 0);
}

bool D3D11Renderer::InitImGui()
{
    m_ImGuiInitialized = ImGui_ImplDX11_Init(m_Device.Get(), m_Context.Get());
    return m_ImGuiInitialized;
}

void D3D11Renderer::ShutdownImGui()
{
    if (m_ImGuiInitialized)
    {
        ImGui_ImplDX11_Shutdown();
        m_ImGuiInitialized = false;
    }
}

void D3D11Renderer::ImGuiNewFrame()
{
    ImGui_ImplDX11_NewFrame();
}

void D3D11Renderer::ImGuiRender(ImDrawData* drawData)
{
    ImGui_ImplDX11_RenderDrawData(drawData);
}

ImTextureID D3D11Renderer::CreateTexture(UINT width, UINT height)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 0;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    Texture tex;
    tex.mipmapped = true;
    HRESULT hr = m_Device->CreateTexture2D(&desc, nullptr, &tex.texture);
    if (FAILED(hr))
    {
        desc.MipLevels = 1;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags = 0;
        tex.mipmapped = false;
        hr = m_Device->CreateTexture2D(&desc, nullptr, &tex.texture);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_Device->CreateShaderResourceView(tex.texture.Get(), nullptr, &tex.srv);
    }
    if (FAILED(hr))
    {
        Log::instance().getAppLogger()->error("[D3D11] Create texture failed: 0x{:08X}", static_cast<uint32_t>(hr));
        return ImTextureID_Invalid;
    }
    const auto id = static_cast<ImTextureID>(reinterpret_cast<intptr_t>(tex.srv.Get()));
    m_Textures.emplace(id, std::move(tex));
    return id;
}

void D3D11Renderer::UpdateTexture(ImTextureID texture, const void* rgba, UINT pitch)
{
    auto it = m_Textures.find(texture);
    if (it == m_Textures.end())
    {
        return;
    }
    m_Context->UpdateSubresource(it->second.texture.Get(), 0, nullptr, rgba, pitch, 0);
    if (it->second.mipmapped)
    {
        m_Context->GenerateMips(it->second.srv.Get());
    }
}

void D3D11Renderer::SetMipSamplerCallback(const ImDrawList*, const ImDrawCmd* cmd)
{
    auto* self = static_cast<D3D11Renderer*>(cmd->UserCallbackData);
    if (self && self->m_MipSampler)
    {
        self->m_Context->PSSetSamplers(0, 1, self->m_MipSampler.GetAddressOf());
    }
}

void D3D11Renderer::BeginSmoothSampling(ImDrawList* drawList)
{
    drawList->AddCallback(&D3D11Renderer::SetMipSamplerCallback, this);
}

void D3D11Renderer::EndSmoothSampling(ImDrawList* drawList)
{
    // Backend DX11 của ImGui tự hiểu callback đặc biệt này: đặt lại sampler/state mặc định.
    drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
}

void D3D11Renderer::DestroyTexture(ImTextureID texture)
{
    m_Textures.erase(texture);
}
} // namespace winpod
