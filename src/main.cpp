#define SDL_MAIN_HANDLED
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "network/NetworkManager.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include "graphics/D3D11Renderer.hpp"
#include "graphics/Camera.hpp"
#include "core/MechController.hpp"
#include "physics/PhysicsManager.hpp"
#include "combat/WeaponSystem.hpp"
#include "combat/TargetDummy.hpp"
#include "combat/TargetLockSystem.hpp"
#include "audio/AudioManager.hpp"
#include "vr/VRManager.hpp"

using namespace DirectX;

namespace
{
    class TeeBuf : public std::streambuf
    {
    public:
        TeeBuf(std::streambuf* sb1, std::streambuf* sb2) : m_sb1(sb1), m_sb2(sb2) {}
    protected:
        int overflow(int c) override
        {
            if (c == EOF) return !EOF;
            if (m_sb1) m_sb1->sputc(c);
            if (m_sb2) m_sb2->sputc(c);
            return c;
        }
        int sync() override
        {
            if (m_sb1) m_sb1->pubsync();
            if (m_sb2) m_sb2->pubsync();
            return 0;
        }
    private:
        std::streambuf* m_sb1;
        std::streambuf* m_sb2;
    };
}

int main(int argc, char* argv[])
{
    // Enable logging to both console and overdrive_vr.log
    std::ofstream logFile("overdrive_vr.log", std::ios::trunc);
    std::streambuf* oldCoutBuf = std::cout.rdbuf();
    std::streambuf* oldCerrBuf = std::cerr.rdbuf();
    std::unique_ptr<TeeBuf> teeCout;
    std::unique_ptr<TeeBuf> teeCerr;
    if (logFile.is_open())
    {
        teeCout = std::make_unique<TeeBuf>(oldCoutBuf, logFile.rdbuf());
        teeCerr = std::make_unique<TeeBuf>(oldCerrBuf, logFile.rdbuf());
        std::cout.rdbuf(teeCout.get());
        std::cerr.rdbuf(teeCerr.get());
    }

    std::cout << "[SYSTEM] Overdrive Core Starting..." << std::endl;

    // 1. Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    const int initialWidth = 1280;
    const int initialHeight = 720;

    SDL_Window* window = SDL_CreateWindow(
        "Overdrive Core - AC6 Combat & Weapon System Prototype",
        initialWidth,
        initialHeight,
        SDL_WINDOW_RESIZABLE
    );

    if (!window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    // Relative mouse mode for FPS/TPS mouselook
    SDL_SetWindowRelativeMouseMode(window, true);

    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    HWND hwnd = static_cast<HWND>(SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));

    // 2. Probe OpenXR Subsystem & Query GPU Adapter LUID
    auto vrManager = std::make_unique<Overdrive::VRManager>();
    LUID preferredLuid = {};
    bool openXrDetected = vrManager->PreInitialize(&preferredLuid);

    // 2.5 Initialize DirectX 11 Renderer with matching GPU Adapter
    auto renderer = std::make_unique<Overdrive::D3D11Renderer>();
    if (!renderer->Initialize(hwnd, initialWidth, initialHeight, openXrDetected ? &preferredLuid : nullptr))
    {
        std::cerr << "Failed to initialize D3D11 Renderer." << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // 2.6 Initialize OpenXR Session & Swapchains using matched D3D11 Device
    if (openXrDetected)
    {
        if (vrManager->Initialize(renderer->GetDevice(), renderer->GetContext()))
        {
            std::cout << "[VR] OpenXR Subsystem initialized successfully. HMD is active!" << std::endl;
        }
        else
        {
            std::cout << "[VR] Failed to initialize OpenXR Session. Falling back to Standard Desktop Mode." << std::endl;
        }
    }
    else
    {
        std::cout << "[VR] OpenXR not detected or HMD inactive. Falling back to Standard Desktop Mode." << std::endl;
    }

    // 3. Initialize Jolt Physics System
    auto physics = std::make_unique<Overdrive::PhysicsManager>();
    if (!physics->Initialize())
    {
        std::cerr << "Failed to initialize Jolt Physics." << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // Initialize static scene colliders (Ground, Walls, Pillars, Slopes)
    renderer->GetGridFloor().InitializePhysics(physics.get());

    // 4. Initialize Controller, Camera & Weapons
    Overdrive::MechController mech;
    if (!mech.InitializePhysics(physics.get()))
    {
        std::cerr << "Failed to initialize Mech Physics." << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    Overdrive::Camera camera;
    Overdrive::WeaponSystem weapons;

    // 5. Initialize Target Dummies (Ground, Platform, Aerial)
    std::vector<Overdrive::TargetDummy> targets = {
        Overdrive::TargetDummy("Target-01 Ground",    {  10.0f,  1.2f, 35.0f }, { 1.8f, 2.4f, 1.8f }, 400.0f),
        Overdrive::TargetDummy("Target-02 Platform",  { -25.0f,  7.7f, 48.0f }, { 1.8f, 2.4f, 1.8f }, 500.0f),
        Overdrive::TargetDummy("Target-03 Aerial",    {   0.0f, 12.0f, 55.0f }, { 2.2f, 2.2f, 2.2f }, 300.0f),
    };

    for (auto& target : targets)
    {
        target.InitializePhysics(physics.get());
    }

    // 5.5 Parse Network Settings from CLI and network.ini
    std::string netModeStr = "none";
    std::string connectIp = "127.0.0.1";
    uint16_t netPort = 7777;

    std::ifstream iniFile("network.ini");
    if (iniFile.is_open())
    {
        std::string line;
        while (std::getline(iniFile, line))
        {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos)
            {
                std::string key = line.substr(0, eqPos);
                std::string val = line.substr(eqPos + 1);
                auto trim = [](std::string& s) {
                    size_t start = s.find_first_not_of(" \t\r\n");
                    size_t end = s.find_last_not_of(" \t\r\n");
                    if (start == std::string::npos) s.clear();
                    else s = s.substr(start, end - start + 1);
                };
                trim(key);
                trim(val);
                if (key == "mode") netModeStr = val;
                else if (key == "ip") connectIp = val;
                else if (key == "port") netPort = static_cast<uint16_t>(std::stoi(val));
            }
        }
    }

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--host")
        {
            netModeStr = "host";
        }
        else if (arg == "--connect" && i + 1 < argc)
        {
            netModeStr = "client";
            connectIp = argv[++i];
        }
        else if (arg == "--port" && i + 1 < argc)
        {
            netPort = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
    }

    Overdrive::NetworkManager network;
    network.Initialize();
    network.GetRemoteMech().InitializePhysics(physics.get());

    if (netModeStr == "host")
    {
        network.StartHost(netPort);
        mech.Respawn({ 0.0f, 2.0f, -30.0f }, 0.0f); // South side facing North
    }
    else if (netModeStr == "client")
    {
        network.StartClient(connectIp, netPort);
        mech.Respawn({ 0.0f, 2.0f, 30.0f }, 3.14159265f); // North side facing South
    }

    // Detect gamepads
    SDL_Gamepad* gamepad = nullptr;
    int numJoysticks = 0;
    SDL_JoystickID* joysticks = SDL_GetGamepads(&numJoysticks);
    if (joysticks && numJoysticks > 0)
    {
        gamepad = SDL_OpenGamepad(joysticks[0]);
        if (gamepad)
        {
            std::cout << "[GAMEPAD DETECTED] " << SDL_GetGamepadName(gamepad) << std::endl;
        }
    }

    std::cout << "========================================================" << std::endl;
    std::cout << " Overdrive Core - FCS Targeting & Lock-On System Online!" << std::endl;
    std::cout << "========================================================" << std::endl;
    std::cout << " [WEAPON & TARGETING CONTROLS]" << std::endl;
    std::cout << " - Right Click / RT : Fire Right Arm Rifle (Orange Tracer)" << std::endl;
    std::cout << " - Left Click  / LT : Fire Left Arm Rifle (Cyan Beam)    " << std::endl;
    std::cout << " - R Key / D-Pad Down: Reload Weapons (Mag: 30, 2.8s)    " << std::endl;
    std::cout << " - Middle Click/ R3 : Toggle TARGET ASSIST (Hard Lock-on)" << std::endl;
    std::cout << " - FCS Soft-Lock    : Auto-aims inner red reticle & dist " << std::endl;
    std::cout << " - V Key            : Switch [FPV Cockpit <-> TPS Chase]" << std::endl;
    std::cout << " - H Key            : Start HOST Mode (Listen on Port " << netPort << ")" << std::endl;
    std::cout << " - C Key            : Connect to Client (" << connectIp << ":" << netPort << ")" << std::endl;
    std::cout << "========================================================" << std::endl;

    Overdrive::TargetLockSystem targetLock;
    Overdrive::AudioManager audioManager;
    audioManager.Initialize();

    bool running = true;
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (running)
    {
        Overdrive::MechInputState input;
        bool fireRightRequested = false;
        bool fireLeftRequested  = false;

        // 1. Process OpenXR & SDL Events
        vrManager->PollEvents();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                {
                    running = false;
                }
                else if (event.key.scancode == SDL_SCANCODE_V)
                {
                    camera.ToggleMode();
                    std::cout << "[CAMERA] Switched to: "
                              << (camera.GetMode() == Overdrive::CameraMode::FPV ? "FPV (Cockpit)" : "TPS (Chase)")
                              << std::endl;
                }
                else if (event.key.scancode == SDL_SCANCODE_R)
                {
                    weapons.ReloadBoth(&audioManager);
                }
                else if (event.key.scancode == SDL_SCANCODE_TAB)
                {
                    input.boostToggle = true;
                }
                else if (event.key.scancode == SDL_SCANCODE_LSHIFT || event.key.scancode == SDL_SCANCODE_RSHIFT)
                {
                    input.quickBoost = true;
                }
                else if (event.key.scancode == SDL_SCANCODE_LCTRL || event.key.scancode == SDL_SCANCODE_RCTRL)
                {
                    input.assaultBoost = true;
                }
                else if (event.key.scancode == SDL_SCANCODE_F)
                {
                    targetLock.ToggleHardLock();
                }
                else if (event.key.scancode == SDL_SCANCODE_H)
                {
                    network.StartHost(netPort);
                    mech.Respawn({ 0.0f, 2.0f, -30.0f }, 0.0f);
                }
                else if (event.key.scancode == SDL_SCANCODE_C)
                {
                    network.StartClient(connectIp, netPort);
                    mech.Respawn({ 0.0f, 2.0f, 30.0f }, 3.14159265f);
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    fireRightRequested = true;
                }
                else if (event.button.button == SDL_BUTTON_LEFT)
                {
                    fireLeftRequested = true;
                }
                else if (event.button.button == SDL_BUTTON_MIDDLE)
                {
                    targetLock.ToggleHardLock();
                }
            }
            else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
            {
                if (event.gbutton.button == SDL_GAMEPAD_BUTTON_WEST)
                {
                    input.quickBoost = true;
                }
                else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_EAST)
                {
                    input.boostToggle = true;
                }
                else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_STICK)
                {
                    input.assaultBoost = true;
                }
                else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_STICK)
                {
                    targetLock.ToggleHardLock();
                }
                else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_BACK)
                {
                    camera.ToggleMode();
                }
                else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                {
                    weapons.ReloadBoth(&audioManager);
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                const float mouseSensitivity = 0.0025f;
                input.yawDelta   += event.motion.xrel * mouseSensitivity;
                input.pitchDelta += event.motion.yrel * mouseSensitivity;
            }
            else if (event.type == SDL_EVENT_GAMEPAD_ADDED)
            {
                if (!gamepad)
                {
                    gamepad = SDL_OpenGamepad(event.gdevice.which);
                    if (gamepad)
                    {
                        std::cout << "[GAMEPAD CONNECTED] " << SDL_GetGamepadName(gamepad) << std::endl;
                    }
                }
            }
            else if (event.type == SDL_EVENT_GAMEPAD_REMOVED)
            {
                if (gamepad && event.gdevice.which == SDL_GetJoystickID(SDL_GetGamepadJoystick(gamepad)))
                {
                    std::cout << "[GAMEPAD DISCONNECTED]" << std::endl;
                    SDL_CloseGamepad(gamepad);
                    gamepad = nullptr;
                }
            }
            else if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                renderer->Resize(event.window.data1, event.window.data2);
            }
        }

        // 2. Poll Continuous Keyboard & Mouse Buttons (Hold to fire)
        const bool* keyboard = SDL_GetKeyboardState(nullptr);
        if (keyboard[SDL_SCANCODE_W]) input.moveForward += 1.0f;
        if (keyboard[SDL_SCANCODE_S]) input.moveForward -= 1.0f;
        if (keyboard[SDL_SCANCODE_D]) input.moveRight   += 1.0f;
        if (keyboard[SDL_SCANCODE_A]) input.moveRight   -= 1.0f;
        if (keyboard[SDL_SCANCODE_SPACE]) input.jumpHold = true;

        float mouseX, mouseY;
        SDL_MouseButtonFlags mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
        if (mouseButtons & SDL_BUTTON_RMASK) fireRightRequested = true;
        if (mouseButtons & SDL_BUTTON_LMASK) fireLeftRequested  = true;

        // 3. Poll Gamepad Analog State & Triggers
        if (gamepad)
        {
            float lx = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;
            float ly = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;
            float rx = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f;
            float ry = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f;

            const float deadzone = 0.15f;
            if (std::abs(lx) > deadzone) input.moveRight += lx;
            if (std::abs(ly) > deadzone) input.moveForward -= ly;
            if (std::abs(rx) > deadzone) input.yawDelta += rx * 0.035f;
            if (std::abs(ry) > deadzone) input.pitchDelta += ry * 0.025f;

            if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH))
            {
                input.jumpHold = true;
            }

            // Triggers for shooting
            float lt = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32767.0f;
            float rt = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32767.0f;
            if (lt > 0.3f) fireLeftRequested  = true;
            if (rt > 0.3f) fireRightRequested = true;
        }

        // 4. Calculate Delta Time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // 5. Update Physics Simulation & Mech
        physics->Update(deltaTime);
        mech.Update(deltaTime, input, physics.get(), &audioManager);
        camera.Update(deltaTime, mech);

        // 5.5 Auto-respawn local mech if destroyed
        if (mech.IsDestroyed() && mech.GetRespawnTimer() <= 0.0f)
        {
            float spawnZ = (network.GetRole() == Overdrive::NetworkRole::Client) ? 30.0f : -30.0f;
            float spawnYaw = (network.GetRole() == Overdrive::NetworkRole::Client) ? 3.14159265f : 0.0f;
            mech.Respawn({ 0.0f, 2.0f, spawnZ }, spawnYaw);
        }

        // 5.6 Update Network Manager (Send 60Hz local state, receive remote packets, update RemoteMech)
        network.Update(deltaTime, mech, &weapons, &audioManager, physics.get());

        bool vrActive = (vrManager->IsAvailable() && vrManager->IsSessionRunning());

        // 6. Update 3D Audio Listener
        if (vrActive && vrManager->HasValidTracking())
        {
            audioManager.UpdateListener(vrManager->GetHmdPosition(), vrManager->GetHmdForward(), XMFLOAT3(0.0f, 1.0f, 0.0f));
        }
        else
        {
            float sinY = std::sin(mech.GetYaw());
            float cosY = std::cos(mech.GetYaw());
            float sinP = std::sin(mech.GetPitch());
            float cosP = std::cos(mech.GetPitch());
            XMFLOAT3 camForward = { sinY * cosP, -sinP, cosY * cosP };
            audioManager.UpdateListener(camera.GetEyePosition(), camForward, XMFLOAT3(0.0f, 1.0f, 0.0f));
        }

        // 7. Update Target Lock System (FCS Soft-Lock & Target Assist Hard-Lock with RemoteMech support)
        if (vrActive && vrManager->HasValidTracking())
        {
            XMMATRIX hmdViewProj = XMMatrixMultiply(vrManager->GetHmdView(), vrManager->GetHmdProj());
            XMFLOAT3 hmdPos = vrManager->GetHmdPosition();
            targetLock.Update(deltaTime, camera, mech, targets, input.yawDelta, input.pitchDelta, &audioManager, &hmdViewProj, &hmdPos, &network.GetRemoteMech());
        }
        else
        {
            targetLock.Update(deltaTime, camera, mech, targets, input.yawDelta, input.pitchDelta, &audioManager, nullptr, nullptr, &network.GetRemoteMech());
        }

        // 8. Process Shooting (TargetLock aim target if locked, otherwise camera/HMD look target)
        XMFLOAT3 aimTarget;
        if (vrActive && vrManager->HasValidTracking())
        {
            XMFLOAT3 hmdPos = vrManager->GetHmdPosition();
            XMFLOAT3 hmdFwd = vrManager->GetHmdForward();
            XMFLOAT3 hmdLookTarget = { hmdPos.x + hmdFwd.x * 50.0f, hmdPos.y + hmdFwd.y * 50.0f, hmdPos.z + hmdFwd.z * 50.0f };
            aimTarget = targetLock.GetAimWorldTarget(camera, &hmdLookTarget);
        }
        else
        {
            aimTarget = targetLock.GetAimWorldTarget(camera);
        }

        if (fireRightRequested)
        {
            if (weapons.FireRightArm(mech.GetRightMuzzlePosition(), aimTarget, &audioManager))
            {
                network.SendFireEvent(false, mech.GetRightMuzzlePosition(), aimTarget);
            }
        }
        if (fireLeftRequested)
        {
            if (weapons.FireLeftArm(mech.GetLeftMuzzlePosition(), aimTarget, &audioManager))
            {
                network.SendFireEvent(true, mech.GetLeftMuzzlePosition(), aimTarget);
            }
        }

        // 9. Update Weapons, Projectiles & Targets (Raycasts against remote mech and dummies)
        weapons.Update(deltaTime, physics.get(), targets, &audioManager, &network.GetRemoteMech(), &network);
        for (auto& target : targets)
        {
            target.Update(deltaTime, physics.get());
        }

        // 10. Render Frame (Stereo VR if HMD active, and Desktop window mirror)
        if (vrActive)
        {
            vrManager->RenderFrame(renderer.get(), mech, weapons, targets, targetLock, &network.GetRemoteMech(), &network);
        }

        // Render to Desktop Window (Mirror view)
        // When VR is active, disable desktop VSync to avoid competing with HMD display refresh
        renderer->BeginFrame();
        renderer->RenderScene(camera, mech, weapons, targets, targetLock, &network.GetRemoteMech(), &network);
        renderer->EndFrame(!vrActive);
    }

    network.Shutdown();

    // Cleanup
    if (gamepad)
    {
        SDL_CloseGamepad(gamepad);
    }
    vrManager.reset();
    renderer.reset();
    physics.reset();
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Application closed." << std::endl;
    if (oldCoutBuf) std::cout.rdbuf(oldCoutBuf);
    if (oldCerrBuf) std::cerr.rdbuf(oldCerrBuf);
    return 0;
}
