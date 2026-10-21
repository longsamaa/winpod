#pragma once
#ifndef _CHOOSE_PROFILE_HPP_
#define _CHOOSE_PROFILE_HPP_
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include "audio/Sound.hpp"
namespace winpod
{
constexpr float kComboTextScale = 0.7f;
class ChooseProfile
{
public:
    ChooseProfile();
    ~ChooseProfile();

public:
    void SetCombobox(const std::vector<SoundDevice>& ins, const std::vector<SoundDevice>& outs);
    void SetSelected(const std::string& inputId, const std::string& outputId);
    const std::string& SelectedInputId() const { return m_InputId; }
    const std::string& SelectedOutputId() const { return m_OutputId; }

    void registerInputSelectedCallback(const std::function<void(const std::string&)> callback);
    void registerOutputSelectedCallback(const std::function<void(const std::string&)> callback);

public:
    void Update(const float& dt);
    void Render(const float& dt);
    float GetWidth() const;
    float GetHeight() const;

private:
    mutable std::mutex m_Mtx;
    std::vector<SoundDevice> m_Inputs;
    std::vector<SoundDevice> m_Outputs;
    std::string m_PendingInputId;
    std::string m_PendingOutputId;
    std::string m_InputId;
    std::string m_OutputId;

    std::function<void(const std::string&)> onInputSelected{nullptr};
    std::function<void(const std::string&)> onOutputSelected{nullptr};
};
} // namespace winpod
#endif // _CHOOSE_PROFILE_HPP_
