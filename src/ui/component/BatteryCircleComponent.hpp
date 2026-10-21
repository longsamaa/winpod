#ifndef _WINPOD_BATTERY_CIRCLE_COMPONENT_HPP_
#define _WINPOD_BATTERY_CIRCLE_COMPONENT_HPP_
#include "ui/component/Component.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace winpod
{
class BatteryCircleComponent : public Component
{
public:
    enum class ICON_TYPE
    {
        IN_EAR,
        CHARING,
        OPEN_LIB
    };
    struct Entry
    {
        std::string label;
        std::optional<uint8_t> battery;
        bool charging{false};
        bool inEar{false};
        bool isOpenLib{false};
    };
    explicit BatteryCircleComponent(const std::string& name);

public:
    void Update(const float& dt) override {}
    void Unload() override {}
    const float AspectRatio() override;
    void Draw(const ImVec2& size) override;
    float FixedHeight() const override;
    void SetEntries(std::vector<Entry> entries);

private:
    float Diameter() const;
    void DrawIcon(ImDrawList* draw, ImFont* font, const ImVec2& center, float radius, ImU32 color, const float& size,
                  ICON_TYPE type);

private:
    std::vector<Entry> m_Entries;
};
} // namespace winpod
#endif //_WINPOD_BATTERY_CIRCLE_COMPONENT_HPP_
