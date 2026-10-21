#pragma once
#ifndef _NOTIFICATION_CLIENT_HPP_
#define _NOTIFICATION_CLIENT_HPP_
#include "platform/WindowsSdk.hpp"
#include <atomic>
#include <string>
namespace trantor
{
class EventLoop;
}
namespace winpod
{
class Sound;

class NotificationClient final : public IMMNotificationClient
{
public:
    NotificationClient(trantor::EventLoop* loop, Sound* sound) : m_Loop(loop), m_Sound(sound) {}

public:
    // override
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_RefCount; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG count = --m_RefCount;
        if (count == 0)
        {
            delete this;
        }
        return count;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override
    {
        if (iid == __uuidof(IUnknown) || iid == __uuidof(IMMNotificationClient))
        {
            *object = static_cast<IMMNotificationClient*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole, LPCWSTR) override
    {
        if (flow == eCapture)
        {
            // Out
        }
        if (flow == eRender)
        {
            // In
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR device_id, DWORD newState) override;
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR device_id) override
    {
        std::string device_name = winrt::to_string(device_id);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR device_id) override
    {
        std::string device_name = winrt::to_string(device_id);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR, const PROPERTYKEY) override { return S_OK; }

private:
    std::atomic<ULONG> m_RefCount{1};
    trantor::EventLoop* m_Loop{nullptr};
    Sound* m_Sound{nullptr};
};
} // namespace winpod
#endif //_NOTIFICATION_CLIENT_HPP_
