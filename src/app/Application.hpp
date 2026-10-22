#pragma once
#include "audio/Sound.hpp"
#ifndef _APPLICATION_HPP_
#define _APPLICATION_HPP_
#include "bluetooth/AirpodParser.hpp"
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <optional>
#include <functional>
namespace trantor
{
class EventLoop;
}
namespace winpod
{
class BLEWatcher;
class AirpodDevice;
class MicGuard;
class Toast;
class Sound;
class Application
{
public:
    Application(const std::string& name);
    ~Application();

public:
    std::string getName() const;
    void Start();
    void Quit();
    void Render(const float& dpiScale, const float& dt);
    void Init(trantor::EventLoop* loop);
    void InitUI();
    void CreateDevice(const ConnectedAirPods& airpod);
    void SetLoop(trantor::EventLoop* loop);
    void CreateWatcher(trantor::EventLoop* loop);
    void RegisterCallbackBlueToothScan(trantor::EventLoop* loop);
    bool DeviceConnected();
    void UpdateCallback(trantor::EventLoop* loop);
    std::optional<AirpodsStatus> GetAirpodStatus() const;
    void CreateSound(trantor::EventLoop* loop);
    void RegisterDeviceConnectedCallback(
        const std::function<void(const ConnectedAirPods&, const std::vector<SoundDevice>&,
                                 const std::vector<SoundDevice>&)>& callback);
    void RegisterDeviceDisconnectedCallback(const std::function<void()>& callback);
    Sound* GetSound();

private:
    bool IsDeviceConnectedLocked() const;
    void OnDeviceConnected(const ConnectedAirPods& airpod);
    void OnUpdateStatusAirpodDevice(const AirpodsStatus& status);
    void OnDevicesDisconnected(const std::vector<std::string>& containerIds);
    void OnDeviceDestroy();

private:
    mutable std::mutex m_Mtx;
    std::unique_ptr<AirpodDevice> m_Device{nullptr};
    std::shared_ptr<BLEWatcher> m_Watcher{nullptr};
    trantor::EventLoop* m_Loop{nullptr};
    std::unique_ptr<Sound> m_Sound{nullptr};
    std::string m_Name{""};
    double m_IntervalTime{2.0};
    std::shared_ptr<std::atomic<bool>> m_ShuttingDown{std::make_shared<std::atomic<bool>>(false)};
    // Callback
    std::function<void(const ConnectedAirPods&, const std::vector<SoundDevice>&, const std::vector<SoundDevice>&)>
        onDeviceConnectCallback{nullptr};
    std::function<void()> onDeviceDisconnectCallback{nullptr};
};
} // namespace winpod
#endif //_APPLICATION_HPP_
