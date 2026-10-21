#include "Sound.hpp"
#include "platform/WindowsSdk.hpp"
#include "log/Log.hpp"
#include "NotificationClient.hpp"
#include "PolicyConfig.hpp"
#include <format>
#include <trantor/net/EventLoop.h>
namespace winpod
{
Sound::Sound(trantor::EventLoop* loop) : m_Loop(loop) {}
Sound::~Sound() {}
bool Sound::Init(trantor::EventLoop* loop)
{
    auto logger = Log::instance().getAppLogger();
    m_Loop = loop;
    if (m_Enumerator != nullptr)
    {
        logger->trace("[SOUND] Com had register");
        return true;
    }
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&m_Enumerator));
    if (FAILED(hr))
    {
        logger->error("[SOUND] Can't create MMDeviceEnumerator: 0x{:08X}", static_cast<uint32_t>(hr));
        return false;
    }
    m_Client = new NotificationClient(m_Loop, this);
    hr = m_Enumerator->RegisterEndpointNotificationCallback(m_Client);
    if (FAILED(hr))
    {
        logger->error("[Mic] Can't register device notifications: 0x{:08X}", static_cast<uint32_t>(hr));
        return false;
    }
    return true;
}
void Sound::Release()
{
    if (m_Enumerator != nullptr && m_Client != nullptr)
    {
        m_Enumerator->UnregisterEndpointNotificationCallback(m_Client);
    }
    if (m_Client != nullptr)
    {
        m_Client->Release();
        m_Client = nullptr;
    }

    if (m_Enumerator)
    {
        m_Enumerator->Release();
        m_Enumerator = nullptr;
    }
}
void Sound::RegisterDeviceStateChangeCallback(std::function<void(const std::string&)> callback)
{
    onDeviceStateChangeCallback = std::move(callback);
}
void Sound::RegisterDefaultDeviceInputChangeCallback(std::function<void(const std::string&)> callback)
{
    onDefaultDeviceInputChangeCallback = std::move(callback);
}
void Sound::RegisterDefaultDeviceOutputChangeCallback(std::function<void(const std::string&)> callback)
{
    onDefaultDeviceOutputChangeCallback = std::move(callback);
}
void Sound::OnDeviceStateChange(const std::string& id, uint32_t state)
{
    Log::instance().getAppLogger()->debug("[SOUND] Device state changed: {} -> {}", id, state);
    if (onDeviceStateChangeCallback)
    {
        onDeviceStateChangeCallback(id);
    }
}
std::pair<std::string, std::string> Sound::GetDeviceName(IMMDevice* device) const
{
    std::string message, name;
    if (!device)
    {
        message = "[SOUND] GetDeviceName: device is null";
        return {message, name};
    }
    IPropertyStore* props = nullptr;
    HRESULT hr_open = device->OpenPropertyStore(STGM_READ, &props);
    if (FAILED(hr_open))
    {
        message = std::format("[SOUND] Can't open property store : 0x{:08X}", static_cast<uint32_t>(hr_open));
        return {message, name};
    }
    PROPVARIANT value;
    PropVariantInit(&value);
    std::wstring w_name;
    HRESULT hr = props->GetValue(PKEY_Device_FriendlyName, &value);
    if (FAILED(hr))
    {
        message = std::format("[SOUND] Can't get device name : 0x{:08X}", static_cast<uint32_t>(hr));
        props->Release();
        return {message, name};
    }
    if (value.vt == VT_LPWSTR && value.pwszVal)
        w_name = value.pwszVal;
    else
        message = "[SOUND] Device has no friendly name";
    name = winrt::to_string(w_name);
    PropVariantClear(&value);
    props->Release();
    return {message, name};
}
std::pair<std::string, std::string> Sound::GetDeivceId(IMMDevice* device) const
{
    std::string message, result;
    if (!device)
    {
        message = "[SOUND] GetDeivceId: device is null";
        return {message, result};
    }
    LPWSTR id = nullptr;
    const HRESULT hr = device->GetId(&id);
    if (FAILED(hr) || !id)
    {
        message = std::format("[SOUND] Can't get device id : 0x{:08X}", static_cast<uint32_t>(hr));
        return {message, result};
    }
    result = winrt::to_string(id);
    CoTaskMemFree(id);
    return {message, result};
}
std::vector<SoundDevice> Sound::GetDevices(bool isIn)
{
    std::vector<SoundDevice> result;
    auto logger = Log::instance().getAppLogger();
    if (!m_Enumerator)
    {
        logger->error("[SOUND] GetDevices: enumerator is null");
        return result;
    }
    const char* direction = isIn ? "In" : "Out";
    Microsoft::WRL::ComPtr<IMMDeviceCollection> devices;
    const HRESULT hr = m_Enumerator->EnumAudioEndpoints(isIn ? eCapture : eRender, DEVICE_STATE_ACTIVE, &devices);
    if (FAILED(hr))
    {
        logger->error("[SOUND] Can't get devices {} : 0x{:08X}", direction, static_cast<uint32_t>(hr));
        return result;
    }
    UINT count = 0;
    devices->GetCount(&count);
    for (UINT i = 0; i < count; ++i)
    {
        Microsoft::WRL::ComPtr<IMMDevice> device;
        const HRESULT hr_get_item = devices->Item(i, &device);
        if (FAILED(hr_get_item))
        {
            logger->error("[SOUND] Can't get device {} : 0x{:08X}", direction, static_cast<uint32_t>(hr_get_item));
            continue;
        }
        const auto [message_get_id, device_id] = GetDeivceId(device.Get());
        if (!message_get_id.empty())
        {
            logger->error(message_get_id);
            continue;
        }
        const auto [message_get_name, device_name] = GetDeviceName(device.Get());
        if (!message_get_name.empty())
        {
            logger->warn(message_get_name);
        }
        result.push_back({.id = device_id, .name = device_name.empty() ? device_id : device_name});
    }
    return result;
}
SoundDevice Sound::GetDefaultDevice(bool isIn) const
{
    auto logger = Log::instance().getAppLogger();
    if (!m_Enumerator)
    {
        logger->error("[SOUND] GetDefaultDevice: enumerator is null");
        return {};
    }
    const char* direction = isIn ? "In" : "Out";
    Microsoft::WRL::ComPtr<IMMDevice> device;
    const HRESULT hr = m_Enumerator->GetDefaultAudioEndpoint(isIn ? eCapture : eRender, eConsole, &device);
    if (hr == E_NOTFOUND)
    {
        logger->debug("[SOUND] No default device {}", direction);
        return {};
    }
    if (FAILED(hr))
    {
        logger->error("[SOUND] Can't get default device {} : 0x{:08X}", direction, static_cast<uint32_t>(hr));
        return {};
    }
    const auto [message_get_id, device_id] = GetDeivceId(device.Get());
    if (!message_get_id.empty())
    {
        logger->error(message_get_id);
        return {};
    }
    const auto [message_get_name, device_name] = GetDeviceName(device.Get());
    if (!message_get_name.empty())
    {
        logger->warn(message_get_name);
    }
    return {.id = device_id, .name = device_name.empty() ? device_id : device_name};
}
bool Sound::SetDefaultDevice(const std::string& id) const
{
    auto logger = Log::instance().getAppLogger();
    if (id.empty())
    {
        logger->error("[SOUND] SetDefaultDevice: id is empty");
        return false;
    }
    Microsoft::WRL::ComPtr<IPolicyConfig> policy;
    HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigClient), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&policy));
    if (FAILED(hr))
    {
        logger->error("[SOUND] IPolicyConfig is not available: 0x{:08X}", static_cast<uint32_t>(hr));
        return false;
    }
    const winrt::hstring wid = winrt::to_hstring(id);
    bool ok = true;
    for (ERole role : {eConsole, eMultimedia, eCommunications})
    {
        hr = policy->SetDefaultEndpoint(wid.c_str(), role);
        if (FAILED(hr))
        {
            logger->error("[SOUND] SetDefaultEndpoint {} (role {}) failed: 0x{:08X}", id, static_cast<int>(role),
                          static_cast<uint32_t>(hr));
            ok = false;
        }
    }
    if (ok)
    {
        logger->info("[SOUND] Default device -> {}", id);
    }
    return ok;
}
bool Sound::SetDefaultDeviceIn(const std::string& id) const
{
    if (!IsDataFlow(id, true))
    {
        Log::instance().getAppLogger()->error("[SOUND] SetDefaultDeviceIn: {} is not an input device", id);
        return false;
    }
    return SetDefaultDevice(id);
}
bool Sound::SetDefaultDeviceOut(const std::string& id) const
{
    if (!IsDataFlow(id, false))
    {
        Log::instance().getAppLogger()->error("[SOUND] SetDefaultDeviceOut: {} is not an output device", id);
        return false;
    }
    return SetDefaultDevice(id);
}
bool Sound::IsDataFlow(const std::string& id, bool isIn) const
{
    if (!m_Enumerator || id.empty())
    {
        return false;
    }
    Microsoft::WRL::ComPtr<IMMDevice> device;
    if (FAILED(m_Enumerator->GetDevice(winrt::to_hstring(id).c_str(), &device)))
    {
        return false;
    }
    Microsoft::WRL::ComPtr<IMMEndpoint> endpoint;
    if (FAILED(device.As(&endpoint)))
    {
        return false;
    }
    EDataFlow flow{};
    if (FAILED(endpoint->GetDataFlow(&flow)))
    {
        return false;
    }
    return flow == (isIn ? eCapture : eRender);
}
} // namespace winpod
