#pragma once
#ifndef _FRAME_ANIMATION_HPP_
#define _FRAME_ANIMATION_HPP_
#include <filesystem>
#include <imgui.h>
#include <memory>
namespace winpod
{
struct Texture;
class FrameAnimation
{
public:
    FrameAnimation() = default;
    ~FrameAnimation();
    FrameAnimation(const FrameAnimation&) = delete;
    FrameAnimation& operator=(const FrameAnimation&) = delete;
    bool Load(const std::filesystem::path& jsonPath);
    void Unload();
    void Update(float deltaSeconds);
    void Draw(const ImVec2& size) const;
    bool IsLoaded() const;
    float AspectRatio() const;
    const std::filesystem::path& ImagePath() const { return m_ImagePath; }

private:
    std::shared_ptr<Texture> m_Texture;
    std::filesystem::path m_ImagePath;
    int m_FrameWidth{0};
    int m_FrameHeight{0};
    int m_ContentX{0};
    int m_ContentY{0};
    int m_ContentW{0};
    int m_ContentH{0};
    int m_Columns{1};
    int m_FrameCount{0};
    float m_FrameTime{0};
    float m_Elapsed{0};
    int m_Current{0};
};
} // namespace winpod
#endif // _FRAME_ANIMATION_HPP_
