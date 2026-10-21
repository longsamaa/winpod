#include "FrameAnimation.hpp"
#include "log/Log.hpp"
#include "render/IRenderer.hpp"
#include "resource/ResourceManager.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace winpod
{
FrameAnimation::~FrameAnimation()
{
    Unload();
}

bool FrameAnimation::Load(const std::filesystem::path& jsonPath)
{
    Unload();
    auto logger = Log::instance().getAppLogger();
    float fps = 0;
    try
    {
        std::ifstream file(jsonPath);
        const auto json = nlohmann::json::parse(file);
        m_ImagePath = jsonPath.parent_path() / json.at("image").get<std::string>();
        m_FrameWidth = json.at("frameWidth").get<int>();
        m_FrameHeight = json.at("frameHeight").get<int>();
        m_Columns = json.at("columns").get<int>();
        m_FrameCount = json.at("frames").get<int>();
        fps = json.at("fps").get<float>();
        if (const auto content = json.find("content"); content != json.end())
        {
            m_ContentX = content->at("x").get<int>();
            m_ContentY = content->at("y").get<int>();
            m_ContentW = content->at("w").get<int>();
            m_ContentH = content->at("h").get<int>();
        }
        else
        {
            m_ContentX = m_ContentY = 0;
            m_ContentW = m_FrameWidth;
            m_ContentH = m_FrameHeight;
        }
    }
    catch (const std::exception& e)
    {
        logger->error("[Anim] Read {} failed: {}", jsonPath.string(), e.what());
        Unload();
        return false;
    }
    if (m_FrameWidth <= 0 || m_FrameHeight <= 0 || m_Columns <= 0 || m_FrameCount <= 0 || fps <= 0)
    {
        logger->error("[Anim] Invalid values in {}", jsonPath.string());
        Unload();
        return false;
    }
    if (m_ContentX < 0 || m_ContentY < 0 || m_ContentW <= 0 || m_ContentH <= 0 ||
        m_ContentX + m_ContentW > m_FrameWidth || m_ContentY + m_ContentH > m_FrameHeight)
    {
        logger->error("[Anim] \"content\" is outside the {}x{} frame in {}", m_FrameWidth, m_FrameHeight,
                      jsonPath.string());
        Unload();
        return false;
    }

    m_Texture = ResourceManager::instance().LoadTexture(m_ImagePath);
    if (!m_Texture)
    {
        Unload();
        return false;
    }
    const int rows = (m_FrameCount + m_Columns - 1) / m_Columns;
    if (m_Texture->width < m_Columns * m_FrameWidth || m_Texture->height < rows * m_FrameHeight)
    {
        logger->error("[Anim] {} is {}x{}, too small for the layout in {}", m_ImagePath.string(), m_Texture->width,
                      m_Texture->height, jsonPath.string());
        Unload();
        return false;
    }

    m_FrameTime = 1.0f / fps;
    logger->debug("[Anim] Loaded {} frames ({}x{}) from {}", m_FrameCount, m_FrameWidth, m_FrameHeight,
                  jsonPath.string());
    return true;
}

void FrameAnimation::Unload()
{
    m_Texture.reset();
    m_ImagePath.clear();
    m_FrameWidth = m_FrameHeight = 0;
    m_ContentX = m_ContentY = m_ContentW = m_ContentH = 0;
    m_Columns = 1;
    m_FrameCount = 0;
    m_FrameTime = 0;
    m_Elapsed = 0;
    m_Current = 0;
}

void FrameAnimation::Update(float deltaSeconds)
{
    if (!IsLoaded())
    {
        return;
    }
    m_Elapsed += deltaSeconds;
    while (m_Elapsed >= m_FrameTime)
    {
        m_Elapsed -= m_FrameTime;
        m_Current = (m_Current + 1) % m_FrameCount; // lặp lại từ đầu
    }
}

void FrameAnimation::Draw(const ImVec2& size) const
{
    if (!IsLoaded())
    {
        ImGui::Dummy(size);
        return;
    }
    const int col = m_Current % m_Columns;
    const int row = m_Current / m_Columns;
    const float sw = static_cast<float>(m_Texture->width);
    const float sh = static_cast<float>(m_Texture->height);
    const float x0 = static_cast<float>(col * m_FrameWidth + m_ContentX);
    const float y0 = static_cast<float>(row * m_FrameHeight + m_ContentY);
    const ImVec2 uv0{x0 / sw, y0 / sh};
    const ImVec2 uv1{(x0 + m_ContentW) / sw, (y0 + m_ContentH) / sh};
    IRenderer* renderer = ResourceManager::instance().Renderer();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (renderer)
    {
        renderer->BeginSmoothSampling(drawList);
    }
    ImGui::Image(m_Texture->id, size, uv0, uv1);
    if (renderer)
    {
        renderer->EndSmoothSampling(drawList);
    }
}

bool FrameAnimation::IsLoaded() const
{
    return m_Texture && m_Texture->IsValid();
}

float FrameAnimation::AspectRatio() const
{
    return m_ContentH ? static_cast<float>(m_ContentW) / static_cast<float>(m_ContentH) : 1.0f;
}
} // namespace winpod
