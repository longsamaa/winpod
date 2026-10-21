#include "BatteryCircleComponent.hpp"
#include "ui/Colors.hpp"
#include "ui/Icons.hpp"
#include "ui/ImGuiLayer.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
namespace winpod
{

BatteryCircleComponent::BatteryCircleComponent(const std::string& name) : Component(name) {}

void BatteryCircleComponent::SetEntries(std::vector<Entry> entries)
{
    m_Entries = std::move(entries);
}

float BatteryCircleComponent::Diameter() const
{
    return std::floor(ImGui::GetFontSize() * 1.5f);
}

float BatteryCircleComponent::FixedHeight() const
{
    if (m_Entries.empty())
        return 0.0f;
    return Diameter() + ImGui::GetTextLineHeightWithSpacing();
}

void BatteryCircleComponent::DrawIcon(ImDrawList* draw, ImFont* font, const ImVec2& center, float radius, ImU32 color,
                                      const float& scale, ICON_TYPE type)
{
    const char* icon = nullptr;
    switch (type)
    {
    case ICON_TYPE::IN_EAR:
        icon = icons::kHeadphones;
        break;
    case ICON_TYPE::CHARING:
        icon = icons::kBolt;
        break;
    case ICON_TYPE::OPEN_LIB:
        icon = icons::kBoxOpen;
        break;
    }
    if (!icon)
        return;
    const float size = std::max(1.0f, std::round(radius * scale));
    unsigned int codepoint = 0;
    ImTextCharFromUtf8(&codepoint, icon, nullptr);
    const ImFontGlyph* glyph = font->GetFontBaked(size)->FindGlyphNoFallback(static_cast<ImWchar>(codepoint));
    if (!glyph)
        return;
    const ImVec2 pos{std::round(center.x - (glyph->X0 + glyph->X1) * 0.5f),
                     std::round(center.y - (glyph->Y0 + glyph->Y1) * 0.5f)};
    draw->AddText(font, size, pos, color, icon);
}

const float BatteryCircleComponent::AspectRatio()
{
    const float h = FixedHeight();
    if (h <= 0.0f)
        return 1.0f;
    const float n = static_cast<float>(m_Entries.size());
    const float gap = ImGui::GetStyle().ItemSpacing.x;
    return (n * Diameter() + (n - 1.0f) * gap) / h;
}

void BatteryCircleComponent::Draw(const ImVec2& size)
{
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(size);
    if (m_Entries.empty() || size.x <= 0.0f || size.y <= 0.0f)
        return;

    const float scale = std::min(1.0f, size.y / FixedHeight());
    const float d = Diameter() * scale;
    const float r = d * 0.5f;
    const float thickness = std::max(2.0f, d * 0.1f);
    const float gap = ImGui::GetStyle().ItemSpacing.x * scale;
    const float n = static_cast<float>(m_Entries.size());
    const float contentW = n * d + (n - 1.0f) * gap;
    float x = origin.x + std::floor((size.x - contentW) * 0.5f);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    constexpr float kPi = 3.14159265358979f;
    for (const Entry& e : m_Entries)
    {
        const ImVec2 center{x + r, origin.y + r};
        const float ringR = r - thickness * 0.5f;
        draw->AddCircle(center, ringR, colors::kBatteryTrack, 0, thickness);
        if (e.battery && *e.battery > 0)
        {
            const float frac = std::min(100, static_cast<int>(*e.battery)) / 100.0f;
            const ImU32 color = *e.battery >= 20 ? colors::kBatteryNormal : colors::kBatteryLow;
            if (*e.battery >= 100)
            {
                draw->AddCircle(center, ringR, color, 0, thickness);
            }
            else
            {
                const float start = -kPi * 0.5f;
                draw->PathArcTo(center, ringR, start, start + 2.0f * kPi * frac, 60);
                draw->PathStroke(color, ImDrawFlags_None, thickness);
            }
        }
        ImFont* numberFont = ImGuiLayer::SemiBoldFont() ? ImGuiLayer::SemiBoldFont() : ImGui::GetFont();
        bool isDrawIcon = e.inEar || e.charging || e.isOpenLib;
        if (isDrawIcon)
        {
            ICON_TYPE type;
            if (e.inEar)
            {
                type = ICON_TYPE::IN_EAR;
            }
            else if (e.charging)
            {
                type = ICON_TYPE::CHARING;
            }
            else if (e.isOpenLib)
            {
                type = ICON_TYPE::OPEN_LIB;
            }
            DrawIcon(draw, numberFont, center, ringR - thickness, colors::kBatteryText, 0.8f, type);
        }
        const std::string text = e.battery ? std::to_string(*e.battery) + "%" : "--";
        const float baseFont = ImGui::GetFontSize();
        const float fontSize = scale >= 0.9f ? baseFont : std::max(1.0f, std::round(baseFont * scale));
        constexpr float kPercentScale = 0.7f;
        const float percentSize = std::max(1.0f, std::round(fontSize * kPercentScale));
        const ImVec2 textSize = numberFont->CalcTextSizeA(percentSize, FLT_MAX, 0.0f, text.c_str());
        draw->AddText(numberFont, percentSize,
                      {std::floor(center.x - textSize.x * 0.5f), std::floor(origin.y + d + gap * 0.5f)},
                      colors::kBatteryText, text.c_str());
        x += d + gap;
    }
}
} // namespace winpod
