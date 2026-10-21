#include "ClassicScanner.hpp"
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <log/Log.hpp>
#include <core/AirpodParser.hpp>
using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Enumeration;
namespace winpod
{
namespace
{
constexpr wchar_t kPropIsConnected[] = L"System.Devices.Aep.IsConnected";

winrt::hstring PairedSelector(bool connected)
{
    return BluetoothDevice::GetDeviceSelectorFromPairingState(true) +
           (connected ? L" AND System.Devices.Aep.IsConnected:=System.StructuredQueryType.Boolean#True"
                      : L" AND System.Devices.Aep.IsConnected:=System.StructuredQueryType.Boolean#False");
}
} // namespace

winrt::fire_and_forget ClassicScanner::ListConnectedAirPods(std::function<void(std::vector<ConnectedAirPods>)> onDone)
{
    constexpr wchar_t kPropContainerId[] = L"System.Devices.Aep.ContainerId";
    constexpr wchar_t kPropHardwareIds[] = L"System.Devices.HardwareIds";
    std::vector<ConnectedAirPods> result;
    try
    {
        const auto selector = PairedSelector(true);
        auto requestedProperties =
            winrt::single_threaded_vector<winrt::hstring>({kPropContainerId, kPropDeviceAddress});
        const auto devices = co_await DeviceInformation::FindAllAsync(selector, std::move(requestedProperties));

        for (auto const& device : devices)
        {
            std::wcout << "[DEVICE ID] : " << device.Id().c_str() << std::endl;
            std::wcout << device.Name().c_str() << "\n";
            const auto containerProp = device.Properties().TryLookup(kPropContainerId);
            if (!containerProp)
                continue;
            const auto containerId = winrt::unbox_value<winrt::guid>(containerProp);
            std::wcout << "[GUID] -- " << winrt::to_hstring(containerId).c_str() << std::endl;
            const winrt::hstring aqs = L"System.Devices.ContainerId:=\"" + winrt::to_hstring(containerId) + L"\"";
            std::wcout << "[AQS] : " << aqs.c_str() << std::endl;
            const auto childProps = winrt::single_threaded_vector<winrt::hstring>({kPropHardwareIds});
            const auto childrens =
                co_await DeviceInformation::FindAllAsync(aqs, std::move(childProps), DeviceInformationKind::Device);
            std::optional<BluetoothPnpId> pnp;
            AirpodParser parser;
            for (const auto& child : childrens)
            {
                std::wcout << "[CHILDREN] : " << child.Name().c_str() << std::endl;
                const auto idsProp = child.Properties().TryLookup(kPropHardwareIds);
                if (!idsProp)
                    continue;
                auto ids = idsProp.try_as<winrt::Windows::Foundation::IReferenceArray<winrt::hstring>>();
                for (const auto& id : ids.Value())
                {
                    std::wcout << "[CHILDREN ID] : " << id.c_str() << std::endl;
                    pnp = AirpodParser::ParseHardwareId(id);
                    if (pnp)
                    {
                        break;
                    }
                }
                if (pnp)
                {
                    break;
                }
            }
            if (!pnp || pnp->vendorId != 0x004C)
            {
                continue;
            }
            const AirpodsModel model = parser.ToModel(AirpodParser::ModelIdFromProductId(pnp->productId));
            if (model == AirpodsModel::Unknown)
            {
                continue;
            }
            ConnectedAirPods airpods;
            airpods.userName = winrt::to_string(device.Name());
            airpods.address = winrt::to_string(
                winrt::unbox_value_or<winrt::hstring>(device.Properties().TryLookup(kPropDeviceAddress), L""));
            airpods.productId = pnp->productId;
            airpods.model = model;
            airpods.container_id = winrt::to_string(winrt::to_hstring(containerId));
            airpods.modelName = parser.GetDeviceName(model);
            result.push_back(std::move(airpods));
        }
    }
    catch (winrt::hresult_error const& error)
    {
        Log::instance().getAppLogger()->error("[Scanner] List AirPods failed 0x{:08X}: {}",
                                              static_cast<uint32_t>(error.code()), winrt::to_string(error.message()));
    }
    onDone(std::move(result));
}

winrt::fire_and_forget ClassicScanner::ListDisconnectedDevice(std::function<void(std::vector<std::string>)> onDone)
{
    constexpr wchar_t kPropContainerId[] = L"System.Devices.Aep.ContainerId";
    std::vector<std::string> result;
    try
    {
        const auto selector = PairedSelector(false);
        auto requestedProperties = winrt::single_threaded_vector<winrt::hstring>({kPropContainerId, kPropIsConnected});
        const auto devices = co_await DeviceInformation::FindAllAsync(selector, std::move(requestedProperties));
        for (auto const& device : devices)
        {
            std::wcout << "[DISCONNECTD DEVICE ID] : " << device.Id().c_str() << std::endl;
            std::wcout << "[DISCONNECTD DEVICE ID] : " << device.Name().c_str() << std::endl;
            const bool connected = winrt::unbox_value_or<bool>(device.Properties().TryLookup(kPropIsConnected), false);
            const auto containerProp = device.Properties().TryLookup(kPropContainerId);
            if (connected || !containerProp)
                continue;
            const auto containerId = winrt::unbox_value<winrt::guid>(containerProp);
            result.push_back(winrt::to_string(winrt::to_hstring(containerId)));
        }
    }
    catch (winrt::hresult_error const& error)
    {
        Log::instance().getAppLogger()->error("[Scanner] List disconnected failed 0x{:08X}: {}",
                                              static_cast<uint32_t>(error.code()), winrt::to_string(error.message()));
    }
    onDone(std::move(result));
}

winrt::Windows::Foundation::IAsyncOperation<DeviceInformationCollection> ClassicScanner::ListClassicConnected()
{
    auto filter = PairedSelector(true);
    std::vector<winrt::hstring> properties{
        winrt::hstring{kPropVendorId},
        winrt::hstring{kPropProductId},
        winrt::hstring{kPropDeviceAddress},
    };
    return DeviceInformation::FindAllAsync(filter, std::move(properties));
}
} // namespace winpod
