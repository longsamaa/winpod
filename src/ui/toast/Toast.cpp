#include "Toast.hpp"
#include "platform/win32/Window.hpp"
#include "bluetooth/AirpodParser.hpp"
#include "common/Log.hpp"
#include "platform/WindowsSdk.hpp"
#include "ui/toast/ChooseProfile.hpp"
#include "ui/theme/Colors.hpp"
#include "ui/component/BatteryCircleComponent.hpp"
#include "ui/component/FrameAnimationComponent.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
namespace winpod
{
namespace
{
bool CircleCloseButton(const char* id, const ImVec2& center, float radius, float dpiScale)
{
    ImGui::SetCursorScreenPos({center.x - radius, center.y - radius});
    const bool pressed = ImGui::InvisibleButton(id, {radius * 2.0f, radius * 2.0f});
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImU32 bg = held ? colors::kCloseButtonActive : hovered ? colors::kCloseButtonHovered : colors::kCloseButton;
    draw->AddCircleFilled(center, radius, bg, 32);
    const float arm = radius * 0.4f;
    const float thickness = 1.6f * dpiScale;
    const ImU32 fg = hovered || held ? colors::kCloseIconHovered : colors::kCloseIcon;
    draw->AddLine({center.x - arm, center.y - arm}, {center.x + arm, center.y + arm}, fg, thickness);
    draw->AddLine({center.x - arm, center.y + arm}, {center.x + arm, center.y - arm}, fg, thickness);
    return pressed;
}
} // namespace

void SlideAnimation::Update(const float& dt)
{
    const float step = std::min(dt, 1.0f / 30.0f);
    time = std::min(time + step, duration);
}

float SlideAnimation::GetEase() const
{
    const float t = time / duration;
    const float ease = 1.0f - std::pow(1.0f - t, 3.0f);
    return ease;
}

void SlideAnimation::Reset()
{
    duration = 0.3f;
    time = 0.0f;
}

Toast::~Toast()
{
    Shutdown();
}

void Toast::RegisterOnCloseCallback(const std::function<void()> callback)
{
    onToastCloseCallback = std::move(callback);
}

void Toast::ResetAnimation()
{
    m_slideAnimation.Reset();
}

void Toast::Shutdown()
{
    m_Table.Clear();
}

void Toast::LoadAirpodAnimations(const std::string& spriteName)
{
    m_Table.Clear();
    if (spriteName.empty())
    {
        return;
    }
    const std::filesystem::path resources{"resources"};
    const std::filesystem::path budsJson = resources / "buds" / (spriteName + ".json");
    const std::filesystem::path caseJson = resources / "case" / (spriteName + ".json");
    auto logger = Log::instance().getAppLogger();
    std::shared_ptr<FrameAnimationComponent> buds_fr = std::make_shared<FrameAnimationComponent>("buds");
    if (!buds_fr->Load(budsJson))
    {
        logger->error("[Toast] Can't load animation {}", budsJson.string());
    }
    std::shared_ptr<FrameAnimationComponent> case_fr = std::make_shared<FrameAnimationComponent>("case");
    if (!case_fr->Load(caseJson))
    {
        logger->error("[Toast] Can't load animation {}", caseJson.string());
    }
    if (buds_fr)
        m_Table.AddComponent(0, std::move(buds_fr));
    m_Table.AddComponent(0, std::make_shared<BatteryCircleComponent>("buds_battery"));
    if (case_fr)
        m_Table.AddComponent(1, std::move(case_fr));
    m_Table.AddComponent(1, std::make_shared<BatteryCircleComponent>("case_battery"));
}

void Toast::UpdateAirpod(const std::optional<AirpodsStatus>& status)
{
    if (!status.has_value())
        return;
    if (auto buds_fr = m_Table.GetComponent("buds"))
    {
        if (status->left.battery.has_value() || status->right.battery.has_value())
        {
            buds_fr->SetVisible(true);
        }
        else
        {
            buds_fr->SetVisible(false);
        }
    }

    if (auto case_fr = m_Table.GetComponent("case"))
    {
        if (!status->chargingCase.battery.has_value())
        {
            {
                case_fr->SetVisible(false);
            }
        }
        else
        {
            case_fr->SetVisible(true);
        }
    }
    using Entry = BatteryCircleComponent::Entry;
    if (auto budsBattery = std::dynamic_pointer_cast<BatteryCircleComponent>(m_Table.GetComponent("buds_battery")))
    {
        budsBattery->SetEntries({
            Entry{"L", status->left.battery, status->left.isCharging, status->left.isInEar, false},
            Entry{"R", status->right.battery, status->right.isCharging, status->right.isInEar, false},
        });
        const auto buds_fr = m_Table.GetComponent("buds");
        budsBattery->SetVisible(buds_fr && buds_fr->IsVisible());
    }
    if (auto caseBattery = std::dynamic_pointer_cast<BatteryCircleComponent>(m_Table.GetComponent("case_battery")))
    {
        caseBattery->SetEntries({Entry{"", status->chargingCase.battery, status->chargingCase.isCharging, false,
                                       status->chargingCase.isLidOpen}});
        const auto case_fr = m_Table.GetComponent("case");
        caseBattery->SetVisible(case_fr && case_fr->IsVisible());
    }
}

void Toast::DrawTable(const float& dt)
{
    m_Table.Draw(dt);
}

ChooseProfile& Toast::GetChooseProfileSpace()
{
    return m_ChooseProfile;
}

void Toast::Render(const float& dpiScale, const std::optional<AirpodsStatus>& status, const float& dt)
{
    if (ImGui::GetIO().AppFocusLost)
    {
        ImGui::ClosePopupsExceptModals();
    }
    if (status.has_value())
    {
        std::string device_model = AirpodParser::GetDeviceName(status->model);
        if (m_DeviceModel != device_model && device_model != "Unknow")
        {
            m_DeviceModel = device_model;
            LoadAirpodAnimations(AirpodParser::GetSpriteName(status->model));
        }
    }
    else
    {
        m_DeviceModel = "Unknow";
    }
    m_Table.Update(dt);
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    const float rounding = 20.0f * dpiScale;
    m_slideAnimation.Update(dt);
    const float ease = m_slideAnimation.GetEase();
    const float slide = (1.0f - ease) * displaySize.y * dpiScale;
    ImGui::SetNextWindowPos({0, slide});
    ImGui::SetNextWindowSize(displaySize);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, colors::kCardBackground);
    ImGui::Begin("toast", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar);
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float titleFontSize = std::round(style.FontSizeBase * m_kTitleScale);
    // Title
    const float titleHeight = titleFontSize + style.ItemSpacing.y;
    ImGui::BeginChild("Title", {0.0f, titleHeight}, ImGuiChildFlags_None, ImGuiWindowFlags_NoInputs);
    ImGui::PushFont(nullptr, titleFontSize);
    const float titleW = ImGui::CalcTextSize(m_DeviceModel.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + std::max(0.0f, (avail.x - titleW) * 0.5f));
    ImGui::TextUnformatted(m_DeviceModel.c_str());
    ImGui::PopFont();
    ImGui::EndChild();

    // Close button
    const float closeRadius = 9.0f * dpiScale;
    {
        const float closeSlot = 2.0f * closeRadius + style.ItemSpacing.x;
        const float titleNeedW = titleW + 2.0f * closeSlot;
        const float kMinAnimationWidth = ImGui::GetFontSize() * 5.0f;
        const float tableNeedW = m_Table.MinWidth(kMinAnimationWidth);
        m_DesiredWidth = std::ceil(std::max(titleNeedW, tableNeedW) + 2.0f * style.WindowPadding.x);
    }
    const ImVec2 winPos = ImGui::GetWindowPos();
    const ImVec2 winSize = ImGui::GetWindowSize();
    const ImVec2 closeCenter{winPos.x + winSize.x - style.WindowPadding.x - closeRadius,
                             winPos.y + style.WindowPadding.y + closeRadius * 0.5f};
    if (CircleCloseButton("##close", closeCenter, closeRadius, dpiScale))
    {
        if (onToastCloseCallback)
            onToastCloseCallback();
    }

    //  Table
    const float comboH = m_ChooseProfile.GetHeight();
    constexpr int kComboRows = 2;
    const float combosH = std::ceil(kComboRows * comboH + (kComboRows - 1) * style.ItemSpacing.y);
    const float tableTopGap = std::floor(5 * dpiScale);
    const float tableW = std::min(avail.x, std::floor(m_kTableWidth * dpiScale));
    const float tableH = std::max(0.0f, ImGui::GetContentRegionAvail().y - tableTopGap - style.ItemSpacing.y - combosH);
    const float columnX = ImGui::GetCursorPosX() + std::floor(std::max(0.0f, (avail.x - tableW) * 0.5f));
    ImGui::SetCursorPosX(columnX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + tableTopGap);
    ImGui::BeginChild("Table", {tableW, tableH}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    UpdateAirpod(status);
    DrawTable(dt);
    ImGui::EndChild();
    const float profileW = tableW;
    const float profileH = combosH;
    ImGui::SetCursorPos({columnX, ImGui::GetCursorPosY()});
    ImGui::BeginChild("ChooseProfile", {profileW, profileH}, ImGuiChildFlags_None,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    m_ChooseProfile.Render(dt);
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}
} // namespace winpod
