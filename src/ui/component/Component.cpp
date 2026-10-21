#include "Component.hpp"
namespace winpod
{
const std::string& Component::GetName() const
{
    return m_Name;
}
void Component::SetVisible(bool bVisible)
{
    m_bVisible = bVisible;
}

bool Component::IsVisible() const
{
    return m_bVisible;
}
} // namespace winpod
