#include "FrameAnimationComponent.hpp"
namespace winpod
{
FrameAnimationComponent::FrameAnimationComponent(const std::string& name) : Component(name) {}
FrameAnimationComponent::~FrameAnimationComponent() {}
bool FrameAnimationComponent::Load(const std::filesystem::path& jsonPath)
{
    return m_Animation.Load(jsonPath);
}
void FrameAnimationComponent::Update(const float& dt)
{
    m_Animation.Update(dt);
}
void FrameAnimationComponent::Unload()
{
    m_Animation.Unload();
}
void FrameAnimationComponent::Draw(const ImVec2& size)
{
    m_Animation.Draw(size);
}
const float FrameAnimationComponent::AspectRatio()
{
    return m_Animation.AspectRatio();
}
} // namespace winpod
