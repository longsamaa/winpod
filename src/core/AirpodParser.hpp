#pragma once
#ifndef _AIR_POD_PARSER_HPP_
#define _AIR_POD_PARSER_HPP_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace winpod
{

enum class AirpodsModel : uint16_t
{
    Unknown = 0x0000,
    AirPods1 = 0x0220,
    AirPods2 = 0x0F20,
    AirPods3 = 0x1320,
    AirPods4 = 0x1920,
    AirPods4Anc = 0x1B20,
    AirPodsPro = 0x0E20,
    AirPodsPro2 = 0x1420,
    AirPodsPro2UsbC = 0x2420,
    AirPodsMax = 0x0A20,
    AirPodsMaxUsbC = 0x1F20,
};

struct Earbud
{
    bool isLeft{false};
    std::optional<uint8_t> battery;
    bool isCharging{false};
    bool isInEar{false};
    bool isPrimary{false};

    bool operator==(const Earbud&) const = default;
};

struct ChargingCase
{
    std::optional<uint8_t> battery;
    bool isCharging{false};
    bool isLidOpen{false};
    bool operator==(const ChargingCase&) const = default;
};

struct AirpodsStatus
{
    AirpodsModel model{AirpodsModel::Unknown};
    uint16_t modelId{0};
    std::string_view name;
    uint8_t color{0};
    Earbud left{.isLeft = true};
    Earbud right{.isLeft = false};
    ChargingCase chargingCase;
};

struct BluetoothPnpId
{
    uint16_t vendorIdSource{0};
    uint16_t vendorId{0};
    uint16_t productId{0};
};

struct ConnectedAirPods
{
    std::string userName;
    std::string address;
    uint16_t productId{0};
    std::string container_id{""};
    AirpodsModel model{AirpodsModel::Unknown};
    std::string_view modelName;
};

class AirpodParser
{
public:
    AirpodParser();
    ~AirpodParser();

public:
    std::optional<AirpodsStatus> Parse(const uint8_t* data, uint32_t length);
    bool CheckAppleCompanyId(const uint16_t& companyId);
    bool CheckIsAirpod(const uint8_t* data, uint32_t length);
    bool CheckLength(const uint8_t* data);
    uint16_t GetDeviceModel(const uint8_t* data);
    AirpodsModel ToModel(uint16_t modelId);
    static std::string GetDeviceName(AirpodsModel model);
    static std::string GetSpriteName(AirpodsModel model);
    std::optional<uint8_t> GetLeftBattery(const uint8_t* data, uint32_t length);
    std::optional<uint8_t> GetRightBattery(const uint8_t* data, uint32_t length);
    std::optional<uint8_t> GetCaseBattery(const uint8_t* data, uint32_t length);
    bool IsLeftCharging(const uint8_t* data, uint32_t length);
    bool IsRightCharging(const uint8_t* data, uint32_t length);
    bool IsCaseCharging(const uint8_t* data, uint32_t length);
    bool IsFlipped(const uint8_t* data);
    static std::optional<BluetoothPnpId> ParseHardwareId(std::wstring_view hardwareId);
    static uint16_t ModelIdFromProductId(uint16_t productId);

private:
    std::optional<uint8_t> NibbleToPercent(uint8_t nibble);

private:
};
} // namespace winpod
#endif //_AIR_POD_PARSER_HPP_
