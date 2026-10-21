#pragma once
#ifndef _RESOURCE_MANAGER_HPP_
#define _RESOURCE_MANAGER_HPP_
#include <imgui.h>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
namespace winpod
{
class IRenderer;
struct Texture
{
    ImTextureID id{ImTextureID_Invalid};
    int width{0};
    int height{0};

    bool IsValid() const { return id != ImTextureID_Invalid; }
};

class ResourceManager
{
public:
    static ResourceManager& instance();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    void Init(std::shared_ptr<IRenderer> renderer);
    void Shutdown();

public:
    std::shared_ptr<Texture> LoadTexture(const std::filesystem::path& imagePath);
    std::shared_ptr<Texture> GetTexture(const std::filesystem::path& imagePath) const;
    void UnloadTexture(const std::filesystem::path& imagePath);
    void UnloadTexture(const std::shared_ptr<Texture>& texture);
    IRenderer* Renderer() const { return m_Renderer.get(); }

private:
    ResourceManager() = default;
    ~ResourceManager();

    static std::string Key(const std::filesystem::path& imagePath);
    void Destroy(Texture& texture);

private:
    std::shared_ptr<IRenderer> m_Renderer{nullptr};
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
};
} // namespace winpod
#endif //_RESOURCE_MANAGER_HPP_
