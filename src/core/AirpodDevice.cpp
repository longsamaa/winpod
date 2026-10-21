#include "AirpodDevice.hpp"
#include <core/AirpodParser.hpp>
#include <cstdint>
#include <format>
#include <string>
#include <log/Log.hpp>
namespace winpod
{
namespace
{
std::string BatteryText(const std::optional<uint8_t>& battery, bool charging)
{
    return battery ? std::format("{:>3}%{}", *battery, charging ? "+" : " ") : std::string("  -- ");
}

std::string EarbudText(const Earbud& bud)
{
    return BatteryText(bud.battery, bud.isCharging) + (bud.isInEar ? " in-ear" : "");
}
} // namespace

AirpodDevice::AirpodDevice(const ConnectedAirPods& connectedAirpod)
    : m_Model(connectedAirpod.model)
    , m_ModelName(connectedAirpod.modelName)
    , m_ContainerID(connectedAirpod.container_id)
    , m_ProductId(connectedAirpod.productId)
    , m_Address(connectedAirpod.address)
    , m_UserName(connectedAirpod.userName)
{
    Log::instance().getAppLogger()->info("[DEVICE] \"{}\" ({}) created, container {}", m_UserName, m_ModelName,
                                         m_ContainerID);
}

AirpodDevice::~AirpodDevice() = default;

void AirpodDevice::Destroy()
{
    m_bIsConnected.store(false);
    ResetStatus();
    Log::instance().getAppLogger()->trace("[DEVICE] \"{}\" destroyed", m_UserName);
}

void AirpodDevice::ResetStatus()
{
    m_Left = Earbud{.isLeft = true, .battery = false};
    m_Right = Earbud{.isLeft = false, .battery = false};
    m_ChargingCase = ChargingCase{};
    m_LastUpdate.reset();
}

void AirpodDevice::Connect()
{
    if (m_bIsConnected.exchange(true))
    {
        return;
    }
    Log::instance().getAppLogger()->info("[DEVICE] \"{}\" ({}) connected", m_UserName, m_ModelName);
}

void AirpodDevice::Disconnect()
{
    if (!m_bIsConnected.exchange(false))
    {
        return;
    }
    ResetStatus();
    Log::instance().getAppLogger()->info("[DEVICE] \"{}\" ({}) disconnected", m_UserName, m_ModelName);
}

bool AirpodDevice::Disconnected() const
{
    return !m_bIsConnected.load();
}

bool AirpodDevice::IsStale(std::chrono::steady_clock::duration maxAge) const
{
    return !m_LastUpdate || std::chrono::steady_clock::now() - *m_LastUpdate > maxAge;
}

bool AirpodDevice::UpdateStatus(const AirpodsStatus& status)
{
    auto logger = Log::instance().getAppLogger();
    if (Disconnected())
    {
        return false;
    }
    m_LastUpdate = std::chrono::steady_clock::now();
    const bool changed = m_Left != status.left || m_Right != status.right || m_ChargingCase != status.chargingCase ||
                         m_Color != status.color;
    if (!changed)
    {
        return false;
    }
    m_Left = status.left;
    m_Right = status.right;
    m_ChargingCase = status.chargingCase;
    m_Color = status.color;
    return true;
}
} // namespace winpod
