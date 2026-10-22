#include "ChooseProfile.hpp"
#include "ui/theme/Colors.hpp"
#include "imgui.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <string>

namespace winpod
{
ChooseProfile::ChooseProfile() {}
ChooseProfile::~ChooseProfile() {}
void ChooseProfile::Update(const float& dt) {}
void ChooseProfile::Render(const float& dt)
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::kComboBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors::kComboBgHovered);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colors::kComboBgHovered);
    ImGui::PushStyleColor(ImGuiCol_Button, colors::kComboBg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors::kComboBgHovered);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, colors::kComboPopupBg);
    ImGui::PushStyleColor(ImGuiCol_Header, colors::kComboItemSelected);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colors::kComboItemHovered);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, colors::kComboItemHovered);
    ImGui::PushStyleColor(ImGuiCol_Border, colors::kComboBorder);
    const ImGuiStyle& style = ImGui::GetStyle();
    const float titleFontSize = std::round(style.FontSizeBase * kComboTextScale);
    ImGui::PushFont(nullptr, titleFontSize);
    std::vector<SoundDevice> inputs, outputs;
    {
        std::lock_guard lock(m_Mtx);
        inputs = m_Inputs;
        outputs = m_Outputs;
        if (!m_PendingInputId.empty())
        {
            m_InputId = std::move(m_PendingInputId);
            m_PendingInputId.clear();
        }
        if (!m_PendingOutputId.empty())
        {
            m_OutputId = std::move(m_PendingOutputId);
            m_PendingOutputId.clear();
        }
    }
    const float labelW = std::max(ImGui::CalcTextSize("Input").x, ImGui::CalcTextSize("Output").x);
    auto labeledCombo = [labelW, &style](const char* label, const char* id, const std::vector<SoundDevice>& devices,
                                         std::string& selectedId, std::function<void(const std::string&)> callback) {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine(labelW + style.ItemSpacing.x);
        ImGui::SetNextItemWidth(-FLT_MIN);
        auto selected = std::find_if(devices.begin(), devices.end(),
                                     [&selectedId](const SoundDevice& d) { return d.id == selectedId; });
        if (selected == devices.end() && !devices.empty())
        {
            selected = devices.begin();
            selectedId = selected->id;
        }
        const char* preview = selected != devices.end() ? selected->name.c_str() : "(none)";
        if (ImGui::BeginCombo(id, preview))
        {
            for (const SoundDevice& device : devices)
            {
                const bool isSelected = device.id == selectedId;
                const std::string itemId = device.name + "##" + device.id;
                if (ImGui::Selectable(itemId.c_str(), isSelected))
                {
                    selectedId = device.id;
                    if (callback)
                    {
                        callback(selectedId);
                    }
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    };
    labeledCombo("Input", "##input", inputs, m_InputId, onInputSelected);
    labeledCombo("Output", "##output", outputs, m_OutputId, onOutputSelected);
    ImGui::PopFont();
    ImGui::PopStyleColor(10);
}
void ChooseProfile::registerInputSelectedCallback(const std::function<void(const std::string&)> callback)
{
    onInputSelected = std::move(callback);
}
void ChooseProfile::registerOutputSelectedCallback(const std::function<void(const std::string&)> callback)
{
    onOutputSelected = std::move(callback);
}
float ChooseProfile::GetWidth() const
{
    return 0.0f;
}
float ChooseProfile::GetHeight() const
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const float titleFontSize = std::round(style.FontSizeBase * kComboTextScale);
    ImGui::PushFont(nullptr, titleFontSize);
    const float comboH = ImGui::GetFrameHeight();
    ImGui::PopFont();
    return comboH;
}
void ChooseProfile::SetCombobox(const std::vector<SoundDevice>& ins, const std::vector<SoundDevice>& outs)
{
    std::lock_guard lock(m_Mtx);
    m_Inputs = ins;
    m_Outputs = outs;
}
void ChooseProfile::SetSelected(const std::string& inputId, const std::string& outputId)
{
    std::lock_guard lock(m_Mtx);
    m_PendingInputId = inputId;
    m_PendingOutputId = outputId;
}
} // namespace winpod
