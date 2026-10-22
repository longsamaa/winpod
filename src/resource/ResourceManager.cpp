#include "ResourceManager.hpp"
#include <render/IRenderer.hpp>
#include "common/Log.hpp"
#include <cstdint>
#include <fstream>
#include <iterator>
#include <stb_image.h>
#include <vector>
namespace winpod
{
namespace
{
using StbPixels = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;
StbPixels DecodeRgba(const std::filesystem::path& path, UINT& width, UINT& height)
{
    StbPixels pixels(nullptr, &stbi_image_free);
    std::ifstream file(path, std::ios::binary);
    const std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    if (bytes.empty())
    {
        Log::instance().getAppLogger()->error("[Resource] Read {} failed", path.string());
        return pixels;
    }

    int w = 0, h = 0, channels = 0;
    pixels.reset(
        stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &channels, STBI_rgb_alpha));
    if (!pixels)
    {
        Log::instance().getAppLogger()->error("[Resource] Decode {} failed: {}", path.string(), stbi_failure_reason());
        return pixels;
    }
    width = static_cast<UINT>(w);
    height = static_cast<UINT>(h);
    return pixels;
}
} // namespace

ResourceManager& ResourceManager::instance()
{
    static ResourceManager s_instance;
    return s_instance;
}

ResourceManager::~ResourceManager()
{
    Shutdown();
}

void ResourceManager::Init(std::shared_ptr<IRenderer> renderer)
{
    Log::instance().getAppLogger()->trace("Create Resource Manager");
    m_Renderer = std::move(renderer);
}

void ResourceManager::Shutdown()
{
    for (auto& [key, texture] : m_Textures)
    {
        if (texture)
        {
            Destroy(*texture);
        }
    }
    m_Textures.clear();
    m_Renderer = nullptr;
}

std::string ResourceManager::Key(const std::filesystem::path& imagePath)
{
    return imagePath.lexically_normal().generic_string();
}

void ResourceManager::Destroy(Texture& texture)
{
    if (m_Renderer && texture.IsValid())
    {
        m_Renderer->DestroyTexture(texture.id);
    }
    texture.id = ImTextureID_Invalid;
}

std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::filesystem::path& imagePath)
{
    const std::string key = Key(imagePath);
    if (auto it = m_Textures.find(key); it != m_Textures.end())
    {
        return it->second;
    }

    auto logger = Log::instance().getAppLogger();
    if (!m_Renderer)
    {
        logger->error("[Resource] Not initialized (call Init(renderer) first), can't load {}", imagePath.string());
        return nullptr;
    }

    UINT width = 0, height = 0;
    const StbPixels pixels = DecodeRgba(imagePath, width, height);
    if (!pixels)
    {
        return nullptr;
    }

    auto texture = std::make_shared<Texture>();
    texture->id = m_Renderer->CreateTexture(width, height);
    if (!texture->IsValid())
    {
        logger->error("[Resource] Create texture {}x{} for {} failed", width, height, imagePath.string());
        return nullptr;
    }
    m_Renderer->UpdateTexture(texture->id, pixels.get(), width * 4);
    texture->width = static_cast<int>(width);
    texture->height = static_cast<int>(height);
    logger->debug("[Resource] Loaded texture {}x{} from {}", width, height, imagePath.string());

    m_Textures.emplace(key, texture);
    return texture;
}

std::shared_ptr<Texture> ResourceManager::GetTexture(const std::filesystem::path& imagePath) const
{
    const auto it = m_Textures.find(Key(imagePath));
    return it != m_Textures.end() ? it->second : nullptr;
}

void ResourceManager::UnloadTexture(const std::filesystem::path& imagePath)
{
    const auto it = m_Textures.find(Key(imagePath));
    if (it == m_Textures.end())
    {
        return;
    }
    if (it->second)
    {
        Destroy(*it->second);
    }
    m_Textures.erase(it);
}

void ResourceManager::UnloadTexture(const std::shared_ptr<Texture>& texture)
{
    if (!texture)
    {
        return;
    }
    Destroy(*texture);
    std::erase_if(m_Textures, [&texture](const auto& entry) { return entry.second == texture; });
}
} // namespace winpod
