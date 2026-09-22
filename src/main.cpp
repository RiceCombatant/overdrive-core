#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <iostream>
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

int main(int argc, char* argv[])
{
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

    // 2. Initialize DirectX 11 Renderer
    auto renderer = std::make_unique<Overdrive::D3D11Renderer>();
    if (!renderer->Initialize(hwnd, initialWidth, initialHeight))
    {
        std::cerr << "Failed to initialize D3D11 Renderer." << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    // 2.5 Initialize OpenXR PCVR Subsystem (Quest 3S / PCVR)
    auto vrManager = std::make_unique<Overdrive::VRManager>();
    if (vrManager->Initialize(renderer->GetDevice(), renderer->GetContext()))
    {
        std::cout << "[VR] OpenXR Subsystem initialized successfully. HMD is active!" << std::endl;
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
    std::cout << " - Middle Click/ R3 : Toggle TARGET ASSIST (Hard Lock-on)" << std::endl;
    std::cout << " - FCS Soft-Lock    : Auto-aims inner red reticle & dist " << std::endl;
    std::cout << " - V Key            : Switch [FPV Cockpit <-> TPS Chase]" << std::endl;
    std::cout << " - Target Dummies   : 3 targets placed (Ground, Ramp, Air)" << std::endl;
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
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                const float mouseSensitivity = 0.0025f;
                input.yawDelta   += event.motion.xrel * mouseSensitivity;
                input.pitchDelta += event.motion.yrel * mouseSensitivity;
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

        // 6. Update 3D Audio Listener (Camera eye position and forward direction)
        float sinY = std::sin(mech.GetYaw());
        float cosY = std::cos(mech.GetYaw());
        float sinP = std::sin(mech.GetPitch());
        float cosP = std::cos(mech.GetPitch());
        XMFLOAT3 camForward = { sinY * cosP, -sinP, cosY * cosP };
        audioManager.UpdateListener(camera.GetEyePosition(), camForward, XMFLOAT3(0.0f, 1.0f, 0.0f));

        // 7. Update Target Lock System (FCS Soft-Lock & Target Assist Hard-Lock)
        targetLock.Update(deltaTime, camera, mech, targets, input.yawDelta, input.pitchDelta, &audioManager);

        // 8. Process Shooting (TargetLock aim target if locked, otherwise camera look target)
        XMFLOAT3 aimTarget = targetLock.GetAimWorldTarget(camera);
        if (fireRightRequested)
        {
            weapons.FireRightArm(mech.GetRightMuzzlePosition(), aimTarget, &audioManager);
        }
        if (fireLeftRequested)
        {
            weapons.FireLeftArm(mech.GetLeftMuzzlePosition(), aimTarget, &audioManager);
        }

        // 9. Update Weapons, Projectiles & Targets
        weapons.Update(deltaTime, physics.get(), targets, &audioManager);
        for (auto& target : targets)
        {
            target.Update(deltaTime, physics.get());
        }

        // 10. Render Frame (Stereo VR if HMD active, and Desktop window mirror)
        if (vrManager->IsAvailable() && vrManager->IsSessionRunning())
        {
            if (vrManager->BeginFrame(mech))
            {
                vrManager->RenderStereo(renderer.get(), mech, weapons, targets, targetLock);
                vrManager->EndFrame();
            }
        }

        // Render to Desktop Window (Mirror view)
        renderer->BeginFrame();
        renderer->RenderScene(camera, mech, weapons, targets, targetLock);
        renderer->EndFrame();
    }

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
    return 0;
}
