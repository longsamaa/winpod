#pragma once
#include <cstdint>
#include <optional>
#ifndef _WINPOD_BLE_WATCHER_HPP_
#define _WINPOD_BLE_WATCHER_HPP_
#include "platform/WindowsSdk.hpp"
#include <core/AirpodParser.hpp>
#include <atomic>
#include <memory>
#include <optional>
#include <functional>
namespace trantor
{
class EventLoop;
}
namespace winpod
{
class AirpodParser;
struct BLEFilter
{
    int16_t min_rssi{-100};
    int16_t max_rssi{0};

public:
    bool Accepts(int16_t rssi) const { return rssi >= min_rssi && rssi <= max_rssi; }
};

class BLEWatcher
{
public:
    BLEWatcher(trantor::EventLoop* loop, BLEFilter&& filter);
    ~BLEWatcher();

public:
    void BeginScan();
    void Stop();
    winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Devices::Enumeration::DeviceInformationCollection>
    ListLEConnected();

public:
    std::function<void(const AirpodsStatus&)> getStatusCallback_{nullptr};

private:
    std::shared_ptr<winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementWatcher> m_Watcher{
        nullptr};
    std::shared_ptr<AirpodParser> m_Parser{};
    trantor::EventLoop* loop_{nullptr};
    std::shared_ptr<std::atomic<bool>> m_Stopped{std::make_shared<std::atomic<bool>>(true)};
    BLEFilter m_Filter{};
};
} // namespace winpod
#endif // _WINPOD_BLE_WATCHER_HPP_
