#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include "graphics/D3D11Renderer.hpp"
#include "graphics/Camera.hpp"
#include "core/MechController.hpp"
#include "physics/PhysicsManager.hpp"

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
        "Overdrive Core - Jolt Physics Mech Flight & Cockpit Prototype",
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

    // 4. Initialize Controller & Camera
    Overdrive::MechController mech;
    if (!mech.InitializePhysics(physics.get()))
    {
        std::cerr << "Failed to initialize Mech Physics." << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    Overdrive::Camera camera;

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
    std::cout << " Overdrive Core - Jolt Physics Integration Ready!       " << std::endl;
    std::cout << "========================================================" << std::endl;
    std::cout << " - Try boosting into walls or pillars (Hard Collision)  " << std::endl;
    std::cout << " - Try sliding up the cyan ramp (Slope Climbing)       " << std::endl;
    std::cout << " - V Key: Switch [FPV Cockpit <-> TPS Chase]           " << std::endl;
    std::cout << "========================================================" << std::endl;

    bool running = true;
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (running)
    {
        Overdrive::MechInputState input;

        // 1. Process Events
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

        // 2. Poll Continuous Keyboard State
        const bool* keyboard = SDL_GetKeyboardState(nullptr);
        if (keyboard[SDL_SCANCODE_W]) input.moveForward += 1.0f;
        if (keyboard[SDL_SCANCODE_S]) input.moveForward -= 1.0f;
        if (keyboard[SDL_SCANCODE_D]) input.moveRight   += 1.0f;
        if (keyboard[SDL_SCANCODE_A]) input.moveRight   -= 1.0f;
        if (keyboard[SDL_SCANCODE_SPACE]) input.jumpHold = true;

        // 3. Poll Gamepad Analog State
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
        }

        // 4. Calculate Delta Time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // 5. Update Physics Simulation & Mech
        physics->Update(deltaTime);
        mech.Update(deltaTime, input, physics.get());
        camera.Update(deltaTime, mech);

        // 6. Render Frame
        renderer->BeginFrame();
        renderer->RenderScene(camera, mech);
        renderer->EndFrame();
    }

    // Cleanup
    if (gamepad)
    {
        SDL_CloseGamepad(gamepad);
    }
    renderer.reset();
    physics.reset();
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Application closed." << std::endl;
    return 0;
}
