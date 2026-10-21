#include "NotificationClient.hpp"
#include "audio/Sound.hpp"
#include <trantor/net/EventLoop.h>
#include <cstdint>
#include <string>
namespace winpod
{
HRESULT STDMETHODCALLTYPE NotificationClient::OnDeviceStateChanged(LPCWSTR device_id, DWORD newState)
{
    if (!m_Loop || !m_Sound || !device_id)
    {
        return S_OK;
    }
    std::string id = winrt::to_string(device_id);
    Sound* sound = m_Sound;
    m_Loop->queueInLoop([sound, id = std::move(id), state = static_cast<uint32_t>(newState)]() {
        sound->OnDeviceStateChange(id, state);
    });
    return S_OK;
}
} // namespace winpod
