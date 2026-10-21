#pragma once
#ifndef _AIRPOD_DEVICE_HPP_
#define _AIRPOD_DEVICE_HPP_
#include <core/AirpodParser.hpp>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
namespace winpod
{
class AirpodDevice
{
public:
    explicit AirpodDevice(const ConnectedAirPods& connectedAirpod);
    ~AirpodDevice();
    AirpodDevice(const AirpodDevice&) = delete;
    AirpodDevice& operator=(const AirpodDevice&) = delete;

public:
    void Destroy();
    bool UpdateStatus(const AirpodsStatus& status);
    void Connect();
    void Disconnect();
    bool Disconnected() const;
    bool IsStale(std::chrono::steady_clock::duration maxAge) const;
    AirpodsModel Model() const { return m_Model; }
    std::string_view ModelName() const { return m_ModelName; }
    uint16_t ProductId() const { return m_ProductId; }
    const std::string& UserName() const { return m_UserName; }
    const std::string& Address() const { return m_Address; }
    const std::string& getContainerID() const { return m_ContainerID; }
    const Earbud& Left() const { return m_Left; }
    const Earbud& Right() const { return m_Right; }
    const ChargingCase& Case() const { return m_ChargingCase; }
    uint8_t Color() const { return m_Color; }

private:
    void ResetStatus();

private:
    AirpodsModel m_Model{AirpodsModel::Unknown};
    std::string_view m_ModelName;
    std::string m_ContainerID{""};
    uint16_t m_ProductId{0};
    std::string m_Address;
    std::string m_UserName;
    uint8_t m_Color{0};
    Earbud m_Left{.isLeft = true};
    Earbud m_Right{.isLeft = false};
    ChargingCase m_ChargingCase;
    std::optional<std::chrono::steady_clock::time_point> m_LastUpdate;
    std::atomic<bool> m_bIsConnected{true};
};
} // namespace winpod
#endif // _AIRPOD_DEVICE_HPP_
