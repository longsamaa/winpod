#pragma once
#ifndef _SOUND_HPP_
#define _SOUND_HPP_
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <functional>
struct IMMDeviceEnumerator;
struct IMMDevice;
namespace trantor
{
class EventLoop;
}
namespace winpod
{
class NotificationClient;
struct SoundDevice
{
    std::string id{""};
    std::string name{""};
};
class Sound
{
public:
    explicit Sound(trantor::EventLoop* loop);
    ~Sound();
    Sound(const Sound&) = delete;
    Sound& operator=(const Sound&) = delete;
    // Register com
public:
    bool Init(trantor::EventLoop* loop);
    void Release();
    std::vector<SoundDevice> GetDevices(bool isIn);
    SoundDevice GetDefaultDevice(bool isIn) const;
    bool SetDefaultDevice(const std::string& id) const;
    bool SetDefaultDeviceIn(const std::string& id) const;
    bool SetDefaultDeviceOut(const std::string& id) const;
    void RegisterDeviceStateChangeCallback(std::function<void(const std::string&)> callback);
    void RegisterDefaultDeviceInputChangeCallback(std::function<void(const std::string&)> callback);
    void RegisterDefaultDeviceOutputChangeCallback(std::function<void(const std::string&)> callback);
    void OnDeviceStateChange(const std::string& id, uint32_t state);

private:
    std::pair<std::string, std::string> GetDeviceName(IMMDevice* device) const;
    std::pair<std::string, std::string> GetDeivceId(IMMDevice* device) const;
    bool IsDataFlow(const std::string& id, bool isIn) const;

private:
    IMMDeviceEnumerator* m_Enumerator{nullptr};
    NotificationClient* m_Client{nullptr};
    trantor::EventLoop* m_Loop{nullptr};
    // callback
    std::function<void(const std::string&)> onDeviceStateChangeCallback{nullptr};
    std::function<void(const std::string&)> onDefaultDeviceInputChangeCallback{nullptr};
    std::function<void(const std::string&)> onDefaultDeviceOutputChangeCallback{nullptr};
};
} // namespace winpod
#endif //_SOUND_HPP_
