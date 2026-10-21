#include "BleWatcher.hpp"
#include <cassert>
#include <cstdint>
#include <trantor/net/EventLoop.h>
#include <log/Log.hpp>
using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Enumeration;
namespace winpod
{
BLEWatcher::BLEWatcher(trantor::EventLoop* loop, BLEFilter&& filter)
{
    loop_ = loop;
    m_Filter = filter;
    m_Watcher = std::make_shared<Advertisement::BluetoothLEAdvertisementWatcher>();
    if (m_Watcher)
    {
        m_Parser = std::make_shared<AirpodParser>();
        Log::instance().getAppLogger()->trace("[Scanner] Create Bluetooth LE Watcher");
        m_Watcher->Received([this, parser = m_Parser, loop = loop_, stopped = m_Stopped,
                             filter = m_Filter](Advertisement::BluetoothLEAdvertisementWatcher const&,
                                                Advertisement::BluetoothLEAdvertisementReceivedEventArgs const& args) {
            if (stopped->load())
            {
                return;
            }
            const int16_t rssi = args.RawSignalStrengthInDBm();
            if (!filter.Accepts(rssi))
            {
                return;
            }
            auto adv = args.Advertisement();
            for (auto const& manufacturer : adv.ManufacturerData())
            {
                auto companyId = manufacturer.CompanyId();
                if (!parser->CheckAppleCompanyId(companyId))
                    continue;
                auto data = manufacturer.Data();
                auto device_status = parser->Parse(data.data(), data.Length());
                if (!device_status)
                {
                    continue;
                }

                loop->queueInLoop([watcher = this, stopped, status = std::move(*device_status)]() {
                    if (stopped->load())
                    {
                        return;
                    }
                    if (watcher->getStatusCallback_)
                    {
                        watcher->getStatusCallback_(status);
                    }
                });
            }
        });
    }
    else
    {
        Log::instance().getAppLogger()->error("[Scanner] Can't Create Bluetooth LE Watcher");
    }
}
BLEWatcher::~BLEWatcher() {}
void BLEWatcher::BeginScan()
{
    assert(m_Watcher);
    m_Stopped->store(false);
    m_Watcher->Start();
    Log::instance().getAppLogger()->trace("[Scanner] Begin Scan");
}
void BLEWatcher::Stop()
{
    assert(m_Watcher);
    m_Stopped->store(true);
    m_Watcher->Stop();
    Log::instance().getAppLogger()->trace("[Scanner] Stop Scan");
}
winrt::Windows::Foundation::IAsyncOperation<DeviceInformationCollection> BLEWatcher::ListLEConnected()
{
    auto filter = BluetoothLEDevice::GetDeviceSelectorFromConnectionStatus(BluetoothConnectionStatus::Connected);
    return DeviceInformation::FindAllAsync(filter);
}
} // namespace winpod
