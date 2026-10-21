#ifndef _WINPOD_COMPONENT_HPP_
#define _WINPOD_COMPONENT_HPP_
#include "imgui.h"
#include <string>
namespace winpod
{
class Component
{
public:
    virtual ~Component() = default;
    const std::string& GetName() const;
    void SetVisible(bool bVisible);
    bool IsVisible() const;
    virtual void Update(const float& dt) = 0;
    virtual void Unload() = 0;
    virtual const float AspectRatio() = 0;
    virtual void Draw(const ImVec2& size) = 0;
    virtual float FixedHeight() const { return 0.0f; }

protected:
    explicit Component(std::string name) : m_Name(std::move(name)) {}
    std::string m_Name;
    bool m_bVisible{true};
};
} // namespace winpod
#endif //_WINPOD_COMPONENT_HPP_
