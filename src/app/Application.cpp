#include "Application.hpp"
#include "core/AirpodParser.hpp"
#include "platform/WindowsSdk.hpp"
#include "core/ble/BleWatcher.hpp"
#include "core/classic/ClassicScanner.hpp"
#include "audio/Sound.hpp"
#include <log/Log.hpp>
#include <memory>
#include <ui/Toast.hpp>
#include <core/AirpodDevice.hpp>
#include <trantor/net/EventLoop.h>
#include <cassert>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace winpod
{
Application::Application(const std::string& name) : m_Name(name)
{
    auto logger = winpod::Log::instance().getAppLogger();
    logger->trace("[Application] Create App");
}

Application::~Application() {}

std::string Application::getName() const
{
    return m_Name;
}

void Application::SetLoop(trantor::EventLoop* loop)
{
    m_Loop = loop;
}

void Application::CreateWatcher(trantor::EventLoop* loop)
{
    m_Watcher = std::make_shared<BLEWatcher>(loop, BLEFilter{.min_rssi = -100, .max_rssi = -5});
    // register callback update status
    m_Watcher->getStatusCallback_ = [application = this, shuttingDown = m_ShuttingDown](const AirpodsStatus& status) {
        if (shuttingDown->load())
            return;
        application->OnUpdateStatusAirpodDevice(status);
    };
}

void Application::OnDeviceConnected(const ConnectedAirPods& airpod)
{
    std::vector<SoundDevice> device_in, device_out;
    if (m_Sound)
    {
        device_in = m_Sound->GetDevices(true);
        device_out = m_Sound->GetDevices(false);
    }
    // Show window
    if (onDeviceConnectCallback)
        onDeviceConnectCallback(airpod, device_in, device_out);
}
void Application::OnDeviceDestroy() {}

void Application::OnUpdateStatusAirpodDevice(const AirpodsStatus& status)
{
    std::lock_guard lock(m_Mtx);
    if (!IsDeviceConnectedLocked())
        return;
    m_Device->UpdateStatus(status);
}

std::optional<AirpodsStatus> Application::GetAirpodStatus() const
{
    std::lock_guard lock(m_Mtx);
    if (!IsDeviceConnectedLocked())
        return std::nullopt;
    AirpodsStatus status;
    status.model = m_Device->Model();
    status.modelId = static_cast<uint16_t>(m_Device->Model());
    status.name = m_Device->ModelName();
    status.color = m_Device->Color();
    status.left = m_Device->Left();
    status.right = m_Device->Right();
    status.chargingCase = m_Device->Case();
    return status;
}

void Application::RegisterDeviceConnectedCallback(
    const std::function<void(const ConnectedAirPods&, const std::vector<SoundDevice>&,
                             const std::vector<SoundDevice>&)>& callback)
{
    onDeviceConnectCallback = std::move(callback);
}

void Application::RegisterDeviceDisconnectedCallback(const std::function<void()>& callback)
{
    onDeviceDisconnectCallback = std::move(callback);
}

void Application::RegisterCallbackBlueToothScan(trantor::EventLoop* loop)
{
    if (!loop)
        return;
    // register callback scan bluetooth
    loop->runEvery(m_IntervalTime, [watcher = m_Watcher, loop = loop, application = this,
                                    shuttingDown = m_ShuttingDown]() {
        if (shuttingDown->load() || application->DeviceConnected())
            return;

        ClassicScanner::ListConnectedAirPods([loop, application, shuttingDown](std::vector<ConnectedAirPods> airpods) {
            if (shuttingDown->load())
                return;
            loop->queueInLoop([airpods = std::move(airpods), application, shuttingDown] {
                if (shuttingDown->load() || airpods.empty())
                    return;
                application->CreateDevice(airpods[0]);
            });
        });
    });
    // register callback scan disconnected bluetooth
    loop->runEvery(m_IntervalTime,
                   [watcher = m_Watcher, loop = loop, application = this, shuttingDown = m_ShuttingDown]() {
                       if (shuttingDown->load())
                           return;

                       ClassicScanner::ListDisconnectedDevice(
                           [loop, application, shuttingDown](std::vector<std::string> container_ids) {
                               if (shuttingDown->load())
                                   return;
                               loop->queueInLoop([container_ids = std::move(container_ids), application, shuttingDown] {
                                   if (shuttingDown->load() || container_ids.empty())
                                       return;
                                   application->OnDevicesDisconnected(container_ids);
                               });
                           });
                   });
}

void Application::CreateSound(trantor::EventLoop* loop)
{
    if (!m_Sound)
    {
        m_Sound = std::make_unique<Sound>(loop);
    }
    m_Sound->Init(loop);
}

void Application::Init(trantor::EventLoop* loop)
{
    SetLoop(loop);
    CreateWatcher(loop);
    RegisterCallbackBlueToothScan(loop);
    CreateSound(loop);
}

void Application::UpdateCallback(trantor::EventLoop* loop) {}

void Application::OnDevicesDisconnected(const std::vector<std::string>& containerIds)
{
    std::lock_guard lock(m_Mtx);
    if (!IsDeviceConnectedLocked())
        return;
    for (const auto& id : containerIds)
    {
        if (id == m_Device->getContainerID())
        {
            m_Device->Disconnect();
            if (onDeviceDisconnectCallback)
                onDeviceDisconnectCallback();
            return;
        }
    }
}

void Application::CreateDevice(const ConnectedAirPods& airpod)
{
    auto logger = winpod::Log::instance().getAppLogger();
    logger->info("[Classic]   {} (PID 0x{:04X})  name=\"{}\"  addr={}", airpod.modelName, airpod.productId,
                 airpod.userName, airpod.address.empty() ? "?" : airpod.address);
    {
        std::lock_guard lock(m_Mtx);
        if (m_Device && m_Device->getContainerID() == airpod.container_id)
        {
            m_Device->Connect();
        }
        else
        {
            m_Device = std::make_unique<AirpodDevice>(airpod);
        }
    }
    OnDeviceConnected(airpod);
}

Sound* Application::GetSound()
{
    return m_Sound.get();
}

bool Application::DeviceConnected()
{
    std::lock_guard lock(m_Mtx);
    return IsDeviceConnectedLocked();
}

bool Application::IsDeviceConnectedLocked() const
{
    return m_Device != nullptr && !m_Device->Disconnected();
}

void Application::Start()
{
    auto logger = winpod::Log::instance().getAppLogger();
    if (!m_Watcher)
    {
        logger->error("Watcher null");
        return;
    }
    logger->trace("[Application] start");
    m_Watcher->BeginScan();
}

void Application::Quit()
{
    m_ShuttingDown->store(true);
    auto logger = winpod::Log::instance().getAppLogger();
    if (!m_Watcher)
    {
        logger->error("Watcher null");
        return;
    }
    bool hadDevice = false;
    {
        std::lock_guard lock(m_Mtx);
        if (m_Device)
        {
            m_Device->Destroy();
            m_Device.reset();
            hadDevice = true;
        }
    }
    if (m_Sound)
    {
        m_Sound->Release();
    }
    if (hadDevice)
    {
        OnDeviceDestroy();
    }
    logger->trace("[Application] quit");
    m_Watcher->Stop();
}
} // namespace winpod
