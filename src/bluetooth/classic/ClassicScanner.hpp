#pragma once
#ifndef _WINPOD_CLASSIC_SCANNER_HPP_
#define _WINPOD_CLASSIC_SCANNER_HPP_
#include "bluetooth/AirpodParser.hpp"
#include <functional>
#include <string>
#include <vector>
#include <platform/WindowsSdk.hpp>
namespace winpod
{
inline constexpr wchar_t kPropVendorId[] = L"System.DeviceInterface.Bluetooth.VendorId";
inline constexpr wchar_t kPropProductId[] = L"System.DeviceInterface.Bluetooth.ProductId";
inline constexpr wchar_t kPropDeviceAddress[] = L"System.Devices.Aep.DeviceAddress";

class ClassicScanner
{
public:
    static winrt::Windows::Foundation::IAsyncOperation<
        winrt::Windows::Devices::Enumeration::DeviceInformationCollection>
    ListClassicConnected();

    static winrt::fire_and_forget ListConnectedAirPods(std::function<void(std::vector<ConnectedAirPods>)> onDone);
    static winrt::fire_and_forget ListDisconnectedDevice(std::function<void(std::vector<std::string>)> onDone);
};
} // namespace winpod
#endif // _WINPOD_CLASSIC_SCANNER_HPP_
