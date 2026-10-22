#ifndef _TOAST_HPP_
#define _TOAST_HPP_
#include "bluetooth/AirpodParser.hpp"
#include "ui/toast/Table.hpp"
#include "ui/toast/ChooseProfile.hpp"
#include <optional>
#include <string>
#include <functional>
namespace winpod
{
class Window;

struct SlideAnimation
{
    float duration{0.3f};
    float time{0.0f};

public:
    void Update(const float& dt);
    void Reset();
    float GetEase() const;
};

class Toast
{
public:
    Toast() = default;
    ~Toast();

public:
    void Shutdown();
    void Render(const float& dpiScale, const std::optional<AirpodsStatus>& status, const float& dt);
    float DesiredWidth() const { return m_DesiredWidth; }
    void ResetAnimation();
    void RegisterOnCloseCallback(const std::function<void()> callback);
    ChooseProfile& GetChooseProfileSpace();

private:
    void LoadAirpodAnimations(const std::string& spriteName);
    void DrawTable(const float& dt);
    void UpdateAirpod(const std::optional<AirpodsStatus>& status);

private:
    Table m_Table;
    ChooseProfile m_ChooseProfile;
    std::string m_DeviceModel{"Unknow"};
    float m_DesiredWidth{0.0f};
    float m_kTitleScale{1.5f};
    float m_kTableWidth{180.0f};
    SlideAnimation m_slideAnimation{0.25f, 0.0};
    // Callback
    std::function<void()> onToastCloseCallback{nullptr};
};
} // namespace winpod
#endif //_TOAST_HPP_
