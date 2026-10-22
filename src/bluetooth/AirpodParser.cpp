#include "AirpodParser.hpp"
#include <cwctype>
namespace winpod
{
namespace
{
std::optional<uint32_t> ParseHex(std::wstring_view text, size_t digits)
{
    if (text.size() < digits)
    {
        return std::nullopt;
    }
    uint32_t value = 0;
    for (size_t i = 0; i < digits; ++i)
    {
        const wchar_t c = static_cast<wchar_t>(std::towupper(text[i]));
        uint32_t digit = 0;
        if (c >= L'0' && c <= L'9')
        {
            digit = static_cast<uint32_t>(c - L'0');
        }
        else if (c >= L'A' && c <= L'F')
        {
            digit = static_cast<uint32_t>(c - L'A' + 10);
        }
        else
        {
            return std::nullopt;
        }
        value = (value << 4) | digit;
    }
    return value;
}

size_t FindAfter(std::wstring_view text, std::wstring_view token)
{
    if (text.size() < token.size())
    {
        return std::wstring_view::npos;
    }
    for (size_t i = 0; i + token.size() <= text.size(); ++i)
    {
        bool match = true;
        for (size_t j = 0; j < token.size(); ++j)
        {
            if (std::towupper(text[i + j]) != std::towupper(token[j]))
            {
                match = false;
                break;
            }
        }
        if (match)
        {
            return i + token.size();
        }
    }
    return std::wstring_view::npos;
}
} // namespace

std::optional<BluetoothPnpId> AirpodParser::ParseHardwareId(std::wstring_view hardwareId)
{
    const size_t vidPos = FindAfter(hardwareId, L"VID&");
    if (vidPos == std::wstring_view::npos)
    {
        return std::nullopt;
    }
    const auto vid = ParseHex(hardwareId.substr(vidPos), 8);
    const size_t pidPos = FindAfter(hardwareId.substr(vidPos), L"PID&");
    if (!vid || pidPos == std::wstring_view::npos)
    {
        return std::nullopt;
    }
    const auto pid = ParseHex(hardwareId.substr(vidPos + pidPos), 4);
    if (!pid)
    {
        return std::nullopt;
    }
    BluetoothPnpId id;
    id.vendorIdSource = static_cast<uint16_t>(*vid >> 16);
    id.vendorId = static_cast<uint16_t>(*vid & 0xFFFF);
    id.productId = static_cast<uint16_t>(*pid);
    return id;
}

uint16_t AirpodParser::ModelIdFromProductId(uint16_t productId)
{
    return static_cast<uint16_t>((productId >> 8) | (productId << 8));
}

AirpodParser::AirpodParser() {}
AirpodParser::~AirpodParser() {}
bool AirpodParser::CheckAppleCompanyId(const uint16_t& companyId)
{
    return companyId == 0x004C;
}
bool AirpodParser::CheckIsAirpod(const uint8_t* data, uint32_t length)
{
    return data != nullptr && length == 27 && data[0] == 0x07;
}
bool AirpodParser::CheckLength(const uint8_t* data)
{
    return data[1] == 25;
}
uint16_t AirpodParser::GetDeviceModel(const uint8_t* data)
{
    return static_cast<uint16_t>((data[3] << 8) | data[4]);
}
AirpodsModel AirpodParser::ToModel(uint16_t modelId)
{
    switch (static_cast<AirpodsModel>(modelId))
    {
    case AirpodsModel::AirPods1:
    case AirpodsModel::AirPods2:
    case AirpodsModel::AirPods3:
    case AirpodsModel::AirPods4:
    case AirpodsModel::AirPods4Anc:
    case AirpodsModel::AirPodsPro:
    case AirpodsModel::AirPodsPro2:
    case AirpodsModel::AirPodsPro2UsbC:
    case AirpodsModel::AirPodsMax:
    case AirpodsModel::AirPodsMaxUsbC:
        return static_cast<AirpodsModel>(modelId);
    default:
        return AirpodsModel::Unknown;
    }
}

std::string AirpodParser::GetDeviceName(AirpodsModel model)
{
    switch (model)
    {
    case AirpodsModel::AirPods1:
        return "AirPods";
    case AirpodsModel::AirPods2:
        return "AirPods 2";
    case AirpodsModel::AirPods3:
        return "AirPods 3";
    case AirpodsModel::AirPods4:
        return "AirPods 4";
    case AirpodsModel::AirPods4Anc:
        return "AirPods 4 (ANC)";
    case AirpodsModel::AirPodsPro:
        return "AirPods Pro";
    case AirpodsModel::AirPodsPro2:
        return "AirPods Pro 2";
    case AirpodsModel::AirPodsPro2UsbC:
        return "AirPods Pro 2 (USB-C)";
    case AirpodsModel::AirPodsMax:
        return "AirPods Max";
    case AirpodsModel::AirPodsMaxUsbC:
        return "AirPods Max (USB-C)";
    case AirpodsModel::Unknown:
        break;
    }
    return "Unknown";
}

std::string AirpodParser::GetSpriteName(AirpodsModel model)
{
    switch (model)
    {
    case AirpodsModel::AirPods1:
        return "AirPods_1";
    case AirpodsModel::AirPods2:
        return "AirPods_2";
    case AirpodsModel::AirPods3:
        return "AirPods_3";
    case AirpodsModel::AirPods4:
        return "AirPods_4";
    case AirpodsModel::AirPods4Anc:
        return "AirPods_4_ANC";
    case AirpodsModel::AirPodsPro:
        return "AirPods_Pro";
    case AirpodsModel::AirPodsPro2:
    case AirpodsModel::AirPodsPro2UsbC:
        return "AirPods_Pro_2";
    case AirpodsModel::AirPodsMax:
    case AirpodsModel::AirPodsMaxUsbC:
        return "AirPods_Max";
    case AirpodsModel::Unknown:
        break;
    }
    return "";
}

std::optional<AirpodsStatus> AirpodParser::Parse(const uint8_t* data, uint32_t length)
{
    if (!CheckIsAirpod(data, length) || !CheckLength(data))
    {
        return std::nullopt;
    }

    AirpodsStatus status;
    status.modelId = GetDeviceModel(data);
    status.model = ToModel(status.modelId);
    status.name = GetDeviceName(status.model);
    status.color = data[9];

    const uint8_t flags = data[5];
    const bool primaryIsLeft = !IsFlipped(data);
    Earbud& primary = primaryIsLeft ? status.left : status.right;
    Earbud& other = primaryIsLeft ? status.right : status.left;
    primary.isPrimary = true;
    primary.isInEar = (flags & 0x02) != 0;
    other.isInEar = (flags & 0x08) != 0;

    status.left.battery = GetLeftBattery(data, length);
    status.right.battery = GetRightBattery(data, length);
    status.left.isCharging = IsLeftCharging(data, length);
    status.right.isCharging = IsRightCharging(data, length);

    status.chargingCase.battery = GetCaseBattery(data, length);
    status.chargingCase.isCharging = IsCaseCharging(data, length);
    status.chargingCase.isLidOpen = (data[8] & 0x08) == 0;
    return status;
}
bool AirpodParser::IsFlipped(const uint8_t* data)
{
    return (data[5] & 0x20) == 0;
}
std::optional<uint8_t> AirpodParser::NibbleToPercent(uint8_t nibble)
{
    if (nibble > 10)
    {
        return std::nullopt;
    }
    return static_cast<uint8_t>(nibble * 10);
}
std::optional<uint8_t> AirpodParser::GetLeftBattery(const uint8_t* data, uint32_t length)
{
    if (!CheckIsAirpod(data, length))
    {
        return std::nullopt;
    }
    const uint8_t high = data[6] >> 4;
    const uint8_t low = data[6] & 0x0F;
    return NibbleToPercent(IsFlipped(data) ? high : low);
}
std::optional<uint8_t> AirpodParser::GetRightBattery(const uint8_t* data, uint32_t length)
{
    if (!CheckIsAirpod(data, length))
    {
        return std::nullopt;
    }
    const uint8_t high = data[6] >> 4;
    const uint8_t low = data[6] & 0x0F;
    return NibbleToPercent(IsFlipped(data) ? low : high);
}
std::optional<uint8_t> AirpodParser::GetCaseBattery(const uint8_t* data, uint32_t length)
{
    if (!CheckIsAirpod(data, length))
    {
        return std::nullopt;
    }
    return NibbleToPercent(data[7] & 0x0F);
}
bool AirpodParser::IsLeftCharging(const uint8_t* data, uint32_t length)
{
    if (!CheckIsAirpod(data, length))
    {
        return false;
    }
    const uint8_t flags = data[7] >> 4;
    return (flags & (IsFlipped(data) ? 0b0010 : 0b0001)) != 0;
}
bool AirpodParser::IsRightCharging(const uint8_t* data, uint32_t length)
{
    if (!CheckIsAirpod(data, length))
    {
        return false;
    }
    const uint8_t flags = data[7] >> 4;
    return (flags & (IsFlipped(data) ? 0b0001 : 0b0010)) != 0;
}
bool AirpodParser::IsCaseCharging(const uint8_t* data, uint32_t length)
{
    if (!CheckIsAirpod(data, length))
    {
        return false;
    }
    const uint8_t flags = data[7] >> 4;
    return (flags & 0b0100) != 0;
}
} // namespace winpod
