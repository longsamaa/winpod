#pragma once
#ifndef _RESOURCE_MANAGER_HPP_
#define _RESOURCE_MANAGER_HPP_
#include <imgui.h> // ImTextureID
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
namespace winpod
{
class IRenderer;

// Texture GPU đã load từ file ảnh. ResourceManager sở hữu; nơi khác giữ shared_ptr để dùng.
// Sau UnloadTexture()/Shutdown(), id thành ImTextureID_Invalid: người còn giữ shared_ptr kiểm tra
// IsValid() trước khi vẽ, không vẽ nhầm texture đã huỷ.
struct Texture
{
    ImTextureID id{ImTextureID_Invalid};
    int width{0};
    int height{0};

    bool IsValid() const { return id != ImTextureID_Invalid; }
};

// Load / cache / unload texture từ file ảnh (PNG). Chỉ dùng trên thread của renderer.
class ResourceManager
{
public:
    static ResourceManager& instance();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    void Init(std::shared_ptr<IRenderer> renderer);
    // Huỷ mọi texture, xoá cache, bỏ renderer. Gọi trước khi renderer bị huỷ.
    void Shutdown();

public:
    // Load lần đầu (giải mã + tạo texture GPU), các lần sau trả bản trong cache. nullptr nếu lỗi.
    std::shared_ptr<Texture> LoadTexture(const std::filesystem::path& imagePath);
    // Texture đã load, hoặc nullptr. Không load.
    std::shared_ptr<Texture> GetTexture(const std::filesystem::path& imagePath) const;
    // Huỷ texture GPU và xoá khỏi cache.
    void UnloadTexture(const std::filesystem::path& imagePath);
    void UnloadTexture(const std::shared_ptr<Texture>& texture);

    // Renderer đang dùng (nullptr trước Init / sau Shutdown), để vẽ texture với sampler phù hợp.
    IRenderer* Renderer() const { return m_Renderer.get(); }

private:
    ResourceManager() = default;
    ~ResourceManager();

    static std::string Key(const std::filesystem::path& imagePath);
    void Destroy(Texture& texture);

private:
    std::shared_ptr<IRenderer> m_Renderer{nullptr};
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures; // key = đường dẫn ảnh
};
} // namespace winpod
#endif //_RESOURCE_MANAGER_HPP_
