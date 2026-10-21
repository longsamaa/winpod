#include <filesystem>
#include <memory>
#include "audio/Sound.hpp"
#include "core/AirpodParser.hpp"
#include "render/IRenderer.hpp"
#include "resource/ResourceManager.hpp"
#include "app/Window.hpp"
#include "ui/ImGuiLayer.hpp"
#include "render/d3d11/D3D11Renderer.hpp"
#include "log/Log.hpp"
#include "ui/Toast.hpp"
#include "utils/PathHelper.hpp"
#include "app/Application.hpp"
#include <imgui.h>
#include <trantor/net/EventLoop.h>
#include <trantor/net/EventLoopThread.h>
static std::unique_ptr<trantor::EventLoop> render_loop{nullptr};
static double kFrameInterval = 1.0 / 60.0;
void SetupPath()
{
    auto current_path = winpod::utils::executableDirectory();
    std::filesystem::current_path(current_path);
}

int WINAPI wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetupPath();
    render_loop = std::make_unique<trantor::EventLoop>();
    trantor::EventLoopThread loopThread("winpod-bt");
    trantor::EventLoop* app_loop = loopThread.getLoop();
    //--setup core---
    std::string appName = "winpod";
    winpod::Log::instance().init(appName);
    auto logger = winpod::Log::instance().getAppLogger();
    winpod::Application app(appName);
    app.Init(app_loop);
    app_loop->queueInLoop([&app, logger, app_loop]() {
        try
        {
            app.Start();
        }
        catch (const winrt::hresult_error& error)
        {
            logger->error("[Application] WinRT error 0x{:08X}: {}", static_cast<uint32_t>(error.code()),
                          winrt::to_string(error.message()));
            app_loop->quit();
        }
    });
    loopThread.run();
    //----------------------
    //--setup UI------
    winpod::Window main_window;
    if (!main_window.Create(appName, hInst))
    {
        logger->error("[WINDOW] Can't Create Main Window {}", appName);
        return -1;
    }
    winpod::ImGuiLayer imgui_layer;
    std::shared_ptr<winpod::IRenderer> renderer = std::make_shared<winpod::D3D11Renderer>();
    if (!renderer->Create(main_window.GetHwnd()))
    {
        logger->error("[RENDERER] Create Renderer Fail");
        return -1;
    }
    if (!imgui_layer.Init(main_window) || !renderer->InitImGui())
    {
        logger->error("[IMGUI LAYER] Init ImGui Fail");
        return 1;
    }
    winpod::ResourceManager::instance().Init(renderer);
    winpod::Toast toast;
    toast.RegisterOnCloseCallback([&main_window]() { main_window.RequestHide(); });
    main_window.registerWindowHideCallback([&toast]() { toast.ResetAnimation(); });
    app.RegisterDeviceConnectedCallback(
        [&main_window, &toast, &app](const winpod::ConnectedAirPods&,
                                     const std::vector<winpod::SoundDevice>& devices_in,
                                     const std::vector<winpod::SoundDevice>& devices_out) {
            if (!main_window.IsShow())
            {
                main_window.Show();
            }
            winpod::ChooseProfile& profile = toast.GetChooseProfileSpace();
            profile.SetCombobox(devices_in, devices_out);
            if (winpod::Sound* sound = app.GetSound())
            {
                profile.SetSelected(sound->GetDefaultDevice(true).id, sound->GetDefaultDevice(false).id);
            }
        });
    if (winpod::Sound* sound = app.GetSound())
    {
        sound->RegisterDeviceStateChangeCallback([sound, &toast](const std::string&) {
            toast.GetChooseProfileSpace().SetCombobox(sound->GetDevices(true), sound->GetDevices(false));
        });
        winpod::ChooseProfile& profile = toast.GetChooseProfileSpace();
        profile.registerInputSelectedCallback([sound, app_loop](const std::string& id) {
            app_loop->queueInLoop([sound, id]() { sound->SetDefaultDeviceIn(id); });
        });
        profile.registerOutputSelectedCallback([sound, app_loop](const std::string& id) {
            app_loop->queueInLoop([sound, id]() { sound->SetDefaultDeviceOut(id); });
        });
    }
    render_loop->runEvery(kFrameInterval, [&main_window, &imgui_layer, renderer, &toast, &app]() {
        main_window.PollMessage();
        if (main_window.Shutdown())
        {
            render_loop->quit();
            return;
        }
        if (!main_window.IsShow())
        {
            return;
        }
        if (!app.DeviceConnected() && !main_window.HidePending())
        {
            main_window.RequestHide();
        }
        const bool hiding = main_window.HidePending();
        UINT width = 0, height = 0;
        if (main_window.ConsumeResize(width, height))
        {
            renderer->Resize(width, height);
        }
        renderer->ImGuiNewFrame();
        imgui_layer.NewFrame();
        const float dpiScale = static_cast<float>(GetDpiForWindow(main_window.GetHwnd())) / USER_DEFAULT_SCREEN_DPI;
        if (!hiding)
        {
            toast.Render(dpiScale, app.GetAirpodStatus(), ImGui::GetIO().DeltaTime);
            main_window.SetClientWidth(static_cast<int>(toast.DesiredWidth()));
        }
        ImDrawData* drawData = imgui_layer.Render();
        const float clearColor[4] = {0, 0, 0, 0};
        renderer->BeginFrame(clearColor);
        renderer->ImGuiRender(drawData);
        renderer->EndFrame();
        if (hiding)
        {
            DwmFlush();
            main_window.Hide();
        }
    });
    //-----------------
    int exitCode = 0;
    try
    {
        render_loop->loop();
        app_loop->queueInLoop([app_loop, &app]() {
            app.Quit();
            app_loop->quit();
        });
        loopThread.wait();
    }
    catch (const std::exception& exception)
    {
        logger->error("[Application] {}", exception.what());
        exitCode = -1;
    }
    toast.Shutdown();
    renderer->ShutdownImGui();
    imgui_layer.Shutdown();
    winpod::ResourceManager::instance().Shutdown();
    renderer->Destroy();
    return exitCode;
}
