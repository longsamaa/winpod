#ifndef _WINPOD_FRAME_ANIMATION_COMPONENT_HPP_
#define _WINPOD_FRAME_ANIMATION_COMPONENT_HPP_
#include "ui/component/Component.hpp"
#include "ui/FrameAnimation.hpp"
#include <filesystem>
#include <string>
namespace winpod
{
class FrameAnimationComponent : public Component
{
public:
    explicit FrameAnimationComponent(const std::string& name);
    ~FrameAnimationComponent() override;

public:
    void Update(const float& dt) override;
    void Unload() override;
    const float AspectRatio() override;
    void Draw(const ImVec2& size) override;
    bool Load(const std::filesystem::path& jsonPath);

private:
    FrameAnimation m_Animation;
};
} // namespace winpod
#endif //_WINPOD_FRAME_ANIMATION_COMPONENT_HPP_
