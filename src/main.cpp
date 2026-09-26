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
#include "combat/AIBotMech.hpp"
#include "audio/AudioManager.hpp"
#include "vr/VRManager.hpp"
#include "ui/MenuSystem.hpp"
#include "core/FrameSystem.hpp"
#include "core/InternalSystem.hpp"
#include "core/LoadoutManager.hpp"

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

    // 5. Initialize Target Dummies (Cover, Shelter Roof, Mega Block, Aerial Drone)
    std::vector<Overdrive::TargetDummy> targets = {
        Overdrive::TargetDummy("Target-01 Cover",        {   8.0f,  1.2f,  15.0f }, { 1.8f, 2.4f, 1.8f }, 450.0f),
        Overdrive::TargetDummy("Target-02 Shelter Roof", { -18.0f, 10.3f, -15.0f }, { 1.8f, 2.4f, 1.8f }, 500.0f),
        Overdrive::TargetDummy("Target-03 Mega Block",   {  38.0f, 12.2f, -12.0f }, { 2.0f, 2.4f, 2.0f }, 600.0f),
        Overdrive::TargetDummy("Target-04 Aerial",       {   0.0f, 15.0f,  22.0f }, { 2.2f, 2.2f, 2.2f }, 350.0f),
    };

    for (auto& target : targets)
    {
        target.InitializePhysics(physics.get());
    }

    enum class ArenaMode : uint8_t
    {
        Empty = 0,
        TargetDummies = 1,
        CombatBots = 2
    };

    ArenaMode currentArenaMode = ArenaMode::TargetDummies;

    // Autonomous AI Combat Mechs
    std::vector<Overdrive::AIBotMech> aiBots = {
        Overdrive::AIBotMech(0, "BOT-ALPHA", {  0.0f, 2.0f,  42.0f }, 3.14159265f),
        Overdrive::AIBotMech(1, "BOT-BETA",  { 38.0f, 2.0f,   0.0f }, -1.5707963f),
    };
    for (auto& bot : aiBots)
    {
        bot.InitializePhysics(physics.get());
    }

    std::vector<Overdrive::TargetDummy> emptyTargets;

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

    enum class GameState
    {
        MainMenu,
        Assemble,
        Playing
    };

    GameState gameState = GameState::MainMenu;
    Overdrive::FrameSystem frameSystem;
    Overdrive::InternalSystem internalSystem;

    Overdrive::AssembleTab assembleCurrentTab = Overdrive::AssembleTab::Weapons;
    int assembleSlotIndex = 0;
    float assembleCamAngle = 0.45f;
    float assembleAnimTime = 0.0f;

    Overdrive::NetworkManager network;
    network.Initialize();
    network.InitializeRemotePhysics(physics.get());

    if (netModeStr == "host")
    {
        network.StartHost(netPort);
        mech.Respawn({ 0.0f, 2.0f, -40.0f }, 0.0f); // South side facing North
        gameState = GameState::Playing;
    }
    else if (netModeStr == "client")
    {
        network.StartClient(connectIp, netPort);
        mech.Respawn({ 0.0f, 2.0f, 40.0f }, 3.14159265f); // North side facing South
        gameState = GameState::Playing;
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
    std::cout << " Overdrive Core - Tactical High-Speed Mech Simulator" << std::endl;
    std::cout << "========================================================" << std::endl;
    std::cout << " [MENU NAVIGATION]" << std::endl;
    std::cout << " - W / S / Up / Down / D-Pad : Select Menu Item" << std::endl;
    std::cout << " - Enter / Space / Gamepad A : Confirm Selection" << std::endl;
    std::cout << " - ESC in Game               : Return to Main Menu" << std::endl;
    std::cout << " - G in Game                 : Enter Garage / Assemble" << std::endl;
    std::cout << "========================================================" << std::endl;

    Overdrive::TargetLockSystem targetLock;
    Overdrive::AudioManager audioManager;
    audioManager.Initialize();

    // Load saved custom assemble build if exists, then apply frame & internal tuning to mech physics & FCS
    Overdrive::LoadoutManager::LoadLoadout("loadout.ini", weapons, frameSystem, internalSystem);
    internalSystem.ApplyToMechAndFcs(mech, targetLock, frameSystem);

    DirectX::XMFLOAT3 savedPlayingPos = { 0.0f, 2.0f, -20.0f };
    float savedPlayingYaw = 0.0f;
    bool enteredAssembleFromPlaying = false;

    bool running = true;
    auto lastTime = std::chrono::high_resolution_clock::now();
    float menuCamAngle = 0.0f;

    // Arena Mode switching helper
    auto setArenaMode = [&](ArenaMode mode, bool notifyNet = true) {
        currentArenaMode = mode;
        network.SetSyncedArenaMode(static_cast<uint8_t>(mode));
        if (notifyNet)
        {
            network.BroadcastArenaMode(static_cast<uint8_t>(mode));
        }

        std::string modeName = "TARGET DUMMIES";
        if (mode == ArenaMode::Empty) modeName = "CLEAN ARENA (EMPTY)";
        else if (mode == ArenaMode::CombatBots) modeName = "AUTONOMOUS COMBAT BOTS";

        renderer->ShowArenaModeBanner("ARENA: " + modeName, 2.5f);
        std::cout << "[ARENA MODE] " << modeName << std::endl;
    };

    // Initialize Menu System
    Overdrive::MenuSystem menu;

    menu.AddItem(
        "bot_combat",
        "ARENA (BOT COMBAT)",
        "BATTLE AGAINST AUTONOMOUS ENEMY AC BOTS",
        [&]() {
            gameState = GameState::Playing;
            setArenaMode(ArenaMode::CombatBots, true);
            mech.Respawn({ 0.0f, 2.0f, -20.0f }, 0.0f);
        }
    );

    menu.AddItem(
        "training",
        "ARENA (TARGET DUMMIES)",
        "START PRACTICE SIMULATION IN TEST GROUND",
        [&]() {
            gameState = GameState::Playing;
            setArenaMode(ArenaMode::TargetDummies, true);
            mech.Respawn({ 0.0f, 2.0f, -20.0f }, 0.0f);
        }
    );

    menu.AddItem(
        "empty_arena",
        "ARENA (CLEAN / EMPTY)",
        "PURE 1V1/FFA PVP BATTLE WITHOUT DUMMIES OR BOTS",
        [&]() {
            gameState = GameState::Playing;
            setArenaMode(ArenaMode::Empty, true);
            mech.Respawn({ 0.0f, 2.0f, -20.0f }, 0.0f);
        }
    );

    menu.AddItem(
        "assemble",
        "GARAGE / ASSEMBLE",
        "CUSTOMIZE WEAPONS, FRAME & INTERNAL PARTS",
        [&]() {
            enteredAssembleFromPlaying = false;
            gameState = GameState::Assemble;
            assembleCurrentTab = Overdrive::AssembleTab::Weapons;
            assembleSlotIndex = 0;
            mech.Respawn({ 0.0f, 1.2f, 0.0f }, 0.0f);
            audioManager.PlayMenuConfirm();
        }
    );

    menu.AddItem(
        "host",
        "MULTIPLAYER (HOST)",
        "HOST 4-PLAYER BATTLE ON PORT " + std::to_string(netPort),
        [&]() {
            network.StartHost(netPort);
            mech.Respawn({ 0.0f, 2.0f, -40.0f }, 0.0f);
            gameState = GameState::Playing;
        }
    );

    menu.AddItem(
        "client",
        "MULTIPLAYER (CONNECT)",
        "JOIN BATTLE AT " + connectIp + ":" + std::to_string(netPort),
        [&]() {
            network.StartClient(connectIp, netPort);
            mech.Respawn({ 0.0f, 2.0f, 40.0f }, 3.14159265f);
            gameState = GameState::Playing;
        }
    );

    menu.AddItem(
        "exit",
        "EXIT GAME",
        "SHUTDOWN OVERDRIVE SYSTEM",
        [&]() {
            running = false;
        }
    );

    while (running)
    {
        Overdrive::MechInputState input;
        bool fireRightRequested = false;
        bool fireLeftRequested  = false;
        bool fireRightShoulderRequested = false;
        bool fireLeftShoulderRequested  = false;

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
                if (gameState == GameState::MainMenu)
                {
                    if (event.key.scancode == SDL_SCANCODE_W || event.key.scancode == SDL_SCANCODE_UP)
                    {
                        menu.NavigateUp(&audioManager);
                    }
                    else if (event.key.scancode == SDL_SCANCODE_S || event.key.scancode == SDL_SCANCODE_DOWN)
                    {
                        menu.NavigateDown(&audioManager);
                    }
                    else if (event.key.scancode == SDL_SCANCODE_RETURN || event.key.scancode == SDL_SCANCODE_SPACE)
                    {
                        menu.Confirm(&audioManager);
                    }
                    else if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                    {
                        running = false;
                    }
                }
                else if (gameState == GameState::Assemble)
                {
                    if (event.key.scancode == SDL_SCANCODE_Q)
                    {
                        int tabIdx = (static_cast<int>(assembleCurrentTab) + 2) % 3;
                        assembleCurrentTab = static_cast<Overdrive::AssembleTab>(tabIdx);
                        int maxSlots = (assembleCurrentTab == Overdrive::AssembleTab::Internal) ? 3 : 4;
                        assembleSlotIndex = std::min(assembleSlotIndex, maxSlots - 1);
                        audioManager.PlayMenuMove();
                    }
                    else if (event.key.scancode == SDL_SCANCODE_E)
                    {
                        int tabIdx = (static_cast<int>(assembleCurrentTab) + 1) % 3;
                        assembleCurrentTab = static_cast<Overdrive::AssembleTab>(tabIdx);
                        int maxSlots = (assembleCurrentTab == Overdrive::AssembleTab::Internal) ? 3 : 4;
                        assembleSlotIndex = std::min(assembleSlotIndex, maxSlots - 1);
                        audioManager.PlayMenuMove();
                    }
                    else if (event.key.scancode == SDL_SCANCODE_W || event.key.scancode == SDL_SCANCODE_UP)
                    {
                        int maxSlots = (assembleCurrentTab == Overdrive::AssembleTab::Internal) ? 3 : 4;
                        assembleSlotIndex = (assembleSlotIndex + maxSlots - 1) % maxSlots;
                        audioManager.PlayMenuMove();
                    }
                    else if (event.key.scancode == SDL_SCANCODE_S || event.key.scancode == SDL_SCANCODE_DOWN)
                    {
                        int maxSlots = (assembleCurrentTab == Overdrive::AssembleTab::Internal) ? 3 : 4;
                        assembleSlotIndex = (assembleSlotIndex + 1) % maxSlots;
                        audioManager.PlayMenuMove();
                    }
                    else if (event.key.scancode == SDL_SCANCODE_A || event.key.scancode == SDL_SCANCODE_LEFT)
                    {
                        if (assembleCurrentTab == Overdrive::AssembleTab::Weapons)
                        {
                            weapons.CycleSlotWeapon(static_cast<Overdrive::WeaponSlot>(assembleSlotIndex), -1, &audioManager);
                        }
                        else if (assembleCurrentTab == Overdrive::AssembleTab::Frame)
                        {
                            frameSystem.CycleSlotPart(static_cast<Overdrive::FrameSlot>(assembleSlotIndex), -1, &audioManager);
                            internalSystem.ApplyToMechAndFcs(mech, targetLock, frameSystem);
                        }
                        else
                        {
                            internalSystem.CycleSlotPart(static_cast<Overdrive::InternalSlot>(assembleSlotIndex), -1, &audioManager);
                            internalSystem.ApplyToMechAndFcs(mech, targetLock, frameSystem);
                        }
                        Overdrive::LoadoutManager::SaveLoadout("loadout.ini", weapons, frameSystem, internalSystem);
                    }
                    else if (event.key.scancode == SDL_SCANCODE_D || event.key.scancode == SDL_SCANCODE_RIGHT)
                    {
                        if (assembleCurrentTab == Overdrive::AssembleTab::Weapons)
                        {
                            weapons.CycleSlotWeapon(static_cast<Overdrive::WeaponSlot>(assembleSlotIndex), 1, &audioManager);
                        }
                        else if (assembleCurrentTab == Overdrive::AssembleTab::Frame)
                        {
                            frameSystem.CycleSlotPart(static_cast<Overdrive::FrameSlot>(assembleSlotIndex), 1, &audioManager);
                            internalSystem.ApplyToMechAndFcs(mech, targetLock, frameSystem);
                        }
                        else
                        {
                            internalSystem.CycleSlotPart(static_cast<Overdrive::InternalSlot>(assembleSlotIndex), 1, &audioManager);
                            internalSystem.ApplyToMechAndFcs(mech, targetLock, frameSystem);
                        }
                        Overdrive::LoadoutManager::SaveLoadout("loadout.ini", weapons, frameSystem, internalSystem);
                    }
                    else if (event.key.scancode == SDL_SCANCODE_RETURN || event.key.scancode == SDL_SCANCODE_SPACE)
                    {
                        Overdrive::LoadoutManager::SaveLoadout("loadout.ini", weapons, frameSystem, internalSystem);
                        audioManager.PlayMenuConfirm();
                        gameState = GameState::Playing;
                        if (enteredAssembleFromPlaying)
                        {
                            mech.Respawn(savedPlayingPos, savedPlayingYaw);
                        }
                        else
                        {
                            mech.Respawn({ 0.0f, 2.0f, -20.0f }, 0.0f);
                        }
                    }
                    else if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                    {
                        Overdrive::LoadoutManager::SaveLoadout("loadout.ini", weapons, frameSystem, internalSystem);
                        audioManager.PlayMenuMove();
                        gameState = GameState::MainMenu;
                        mech.Respawn({ 0.0f, 1.2f, 0.0f }, 0.0f);
                    }
                }
                else // GameState::Playing
                {
                    if (event.key.scancode == SDL_SCANCODE_ESCAPE)
                    {
                        gameState = GameState::MainMenu;
                        mech.Respawn({ 0.0f, 1.2f, 0.0f }, 0.0f);
                        audioManager.PlayMenuMove();
                    }
                    else if (event.key.scancode == SDL_SCANCODE_G)
                    {
                        savedPlayingPos = mech.GetPosition();
                        savedPlayingYaw = mech.GetYaw();
                        enteredAssembleFromPlaying = true;
                        gameState = GameState::Assemble;
                        assembleCurrentTab = Overdrive::AssembleTab::Weapons;
                        assembleSlotIndex = 0;
                        mech.Respawn({ 0.0f, 1.2f, 0.0f }, 0.0f);
                        audioManager.PlayMenuConfirm();
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
                    else if (event.key.scancode == SDL_SCANCODE_Q)
                    {
                        fireLeftShoulderRequested = true;
                    }
                    else if (event.key.scancode == SDL_SCANCODE_E)
                    {
                        fireRightShoulderRequested = true;
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
                        mech.Respawn({ 0.0f, 2.0f, -40.0f }, 0.0f);
                    }
                    else if (event.key.scancode == SDL_SCANCODE_C)
                    {
                        network.StartClient(connectIp, netPort);
                        mech.Respawn({ 0.0f, 2.0f, 40.0f }, 3.14159265f);
                    }
                    else if (event.key.scancode == SDL_SCANCODE_M)
                    {
                        uint8_t nextMode = (static_cast<uint8_t>(currentArenaMode) + 1) % 3;
                        setArenaMode(static_cast<ArenaMode>(nextMode), true);
                    }
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                if (gameState == GameState::Playing)
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
            }
            else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)
            {
                if (gameState == GameState::MainMenu)
                {
                    if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
                    {
                        menu.NavigateUp(&audioManager);
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                    {
                        menu.NavigateDown(&audioManager);
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH) // A
                    {
                        menu.Confirm(&audioManager);
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_EAST || event.gbutton.button == SDL_GAMEPAD_BUTTON_BACK) // B
                    {
                        running = false;
                    }
                }
                else if (gameState == GameState::Assemble)
                {
                    if (event.gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)
                    {
                        int tabIdx = (static_cast<int>(assembleCurrentTab) + 2) % 3;
                        assembleCurrentTab = static_cast<Overdrive::AssembleTab>(tabIdx);
                        int maxSlots = (assembleCurrentTab == Overdrive::AssembleTab::Internal) ? 3 : 4;
                        assembleSlotIndex = std::min(assembleSlotIndex, maxSlots - 1);
                        audioManager.PlayMenuMove();
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                    {
                        int tabIdx = (static_cast<int>(assembleCurrentTab) + 1) % 3;
                        assembleCurrentTab = static_cast<Overdrive::AssembleTab>(tabIdx);
                        int maxSlots = (assembleCurrentTab == Overdrive::AssembleTab::Internal) ? 3 : 4;
                        assembleSlotIndex = std::min(assembleSlotIndex, maxSlots - 1);
                        audioManager.PlayMenuMove();
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
                    {
                        int maxSlots = (assembleCurrentTab == Overdrive::AssembleTab::Internal) ? 3 : 4;
                        assembleSlotIndex = (assembleSlotIndex + maxSlots - 1) % maxSlots;
                        audioManager.PlayMenuMove();
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                    {
                        int maxSlots = (assembleCurrentTab == Overdrive::AssembleTab::Internal) ? 3 : 4;
                        assembleSlotIndex = (assembleSlotIndex + 1) % maxSlots;
                        audioManager.PlayMenuMove();
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT)
                    {
                        if (assembleCurrentTab == Overdrive::AssembleTab::Weapons)
                        {
                            weapons.CycleSlotWeapon(static_cast<Overdrive::WeaponSlot>(assembleSlotIndex), -1, &audioManager);
                        }
                        else if (assembleCurrentTab == Overdrive::AssembleTab::Frame)
                        {
                            frameSystem.CycleSlotPart(static_cast<Overdrive::FrameSlot>(assembleSlotIndex), -1, &audioManager);
                            internalSystem.ApplyToMechAndFcs(mech, targetLock, frameSystem);
                        }
                        else
                        {
                            internalSystem.CycleSlotPart(static_cast<Overdrive::InternalSlot>(assembleSlotIndex), -1, &audioManager);
                            internalSystem.ApplyToMechAndFcs(mech, targetLock, frameSystem);
                        }
                        Overdrive::LoadoutManager::SaveLoadout("loadout.ini", weapons, frameSystem, internalSystem);
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
                    {
                        if (assembleCurrentTab == Overdrive::AssembleTab::Weapons)
                        {
                            weapons.CycleSlotWeapon(static_cast<Overdrive::WeaponSlot>(assembleSlotIndex), 1, &audioManager);
                        }
                        else if (assembleCurrentTab == Overdrive::AssembleTab::Frame)
                        {
                            frameSystem.CycleSlotPart(static_cast<Overdrive::FrameSlot>(assembleSlotIndex), 1, &audioManager);
                            internalSystem.ApplyToMechAndFcs(mech, targetLock, frameSystem);
                        }
                        else
                        {
                            internalSystem.CycleSlotPart(static_cast<Overdrive::InternalSlot>(assembleSlotIndex), 1, &audioManager);
                            internalSystem.ApplyToMechAndFcs(mech, targetLock, frameSystem);
                        }
                        Overdrive::LoadoutManager::SaveLoadout("loadout.ini", weapons, frameSystem, internalSystem);
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH || event.gbutton.button == SDL_GAMEPAD_BUTTON_START) // A or Start
                    {
                        Overdrive::LoadoutManager::SaveLoadout("loadout.ini", weapons, frameSystem, internalSystem);
                        audioManager.PlayMenuConfirm();
                        gameState = GameState::Playing;
                        if (enteredAssembleFromPlaying)
                        {
                            mech.Respawn(savedPlayingPos, savedPlayingYaw);
                        }
                        else
                        {
                            mech.Respawn({ 0.0f, 2.0f, -20.0f }, 0.0f);
                        }
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_EAST || event.gbutton.button == SDL_GAMEPAD_BUTTON_BACK) // B or Back
                    {
                        Overdrive::LoadoutManager::SaveLoadout("loadout.ini", weapons, frameSystem, internalSystem);
                        audioManager.PlayMenuMove();
                        gameState = GameState::MainMenu;
                        mech.Respawn({ 0.0f, 1.2f, 0.0f }, 0.0f);
                    }
                }
                else // GameState::Playing
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
                        gameState = GameState::MainMenu;
                        mech.Respawn({ 0.0f, 1.2f, 0.0f }, 0.0f);
                        audioManager.PlayMenuMove();
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN)
                    {
                        weapons.ReloadBoth(&audioManager);
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP)
                    {
                        uint8_t nextMode = (static_cast<uint8_t>(currentArenaMode) + 1) % 3;
                        setArenaMode(static_cast<ArenaMode>(nextMode), true);
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)
                    {
                        fireLeftShoulderRequested = true;
                    }
                    else if (event.gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
                    {
                        fireRightShoulderRequested = true;
                    }
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                if (gameState == GameState::Playing)
                {
                    const float mouseSensitivity = 0.0025f;
                    input.yawDelta   += event.motion.xrel * mouseSensitivity;
                    input.pitchDelta += event.motion.yrel * mouseSensitivity;
                }
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

        // 4. Calculate Delta Time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        bool vrActive = (vrManager->IsAvailable() && vrManager->IsSessionRunning());

        if (gameState == GameState::MainMenu)
        {
            // Update menu animations
            menu.Update(deltaTime);

            // Orbit cinematic camera around idle mech in hangar / cyber grid
            menuCamAngle += deltaTime * 0.28f;
            float camDist = 6.2f;
            float camX = std::sin(menuCamAngle) * camDist;
            float camZ = -std::cos(menuCamAngle) * camDist;
            float camY = 2.2f;
            camera.SetCustomView({ camX, camY, camZ }, { 0.0f, 1.2f, 0.0f });

            // Update Audio listener for menu
            if (vrActive && vrManager->HasValidTracking())
            {
                audioManager.UpdateListener(vrManager->GetHmdPosition(), vrManager->GetHmdForward(), XMFLOAT3(0.0f, 1.0f, 0.0f));
            }
            else
            {
                XMFLOAT3 eyePos = camera.GetEyePosition();
                XMFLOAT3 lookTarget = camera.GetLookTarget();
                XMFLOAT3 fwd = { lookTarget.x - eyePos.x, lookTarget.y - eyePos.y, lookTarget.z - eyePos.z };
                float len = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y + fwd.z * fwd.z);
                if (len > 0.001f) { fwd.x /= len; fwd.y /= len; fwd.z /= len; }
                audioManager.UpdateListener(eyePos, fwd, XMFLOAT3(0.0f, 1.0f, 0.0f));
            }

            // Render Main Menu (VR Stereo or Desktop)
            if (vrActive)
            {
                vrManager->RenderMenuFrame(renderer.get(), menu, camera, mech);
            }

            renderer->BeginFrame();
            renderer->RenderMainMenu(menu, camera, mech);
            renderer->EndFrame(!vrActive);
        }
        else if (gameState == GameState::Assemble)
        {
            assembleAnimTime += deltaTime;
            assembleCamAngle += deltaTime * 0.15f;
            float camDist = 5.6f;
            float camX = std::sin(assembleCamAngle) * camDist;
            float camZ = -std::cos(assembleCamAngle) * camDist;
            float camY = 1.9f;
            camera.SetCustomView({ camX, camY, camZ }, { 0.0f, 1.3f, 0.0f });

            if (vrActive && vrManager->HasValidTracking())
            {
                audioManager.UpdateListener(vrManager->GetHmdPosition(), vrManager->GetHmdForward(), XMFLOAT3(0.0f, 1.0f, 0.0f));
            }
            else
            {
                XMFLOAT3 eyePos = camera.GetEyePosition();
                XMFLOAT3 lookTarget = camera.GetLookTarget();
                XMFLOAT3 fwd = { lookTarget.x - eyePos.x, lookTarget.y - eyePos.y, lookTarget.z - eyePos.z };
                float len = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y + fwd.z * fwd.z);
                if (len > 0.001f) { fwd.x /= len; fwd.y /= len; fwd.z /= len; }
                audioManager.UpdateListener(eyePos, fwd, XMFLOAT3(0.0f, 1.0f, 0.0f));
            }

            renderer->BeginFrame();
            renderer->RenderAssembleMenu(camera, mech, weapons, frameSystem, internalSystem, assembleCurrentTab, assembleSlotIndex, assembleAnimTime);
            renderer->EndFrame(!vrActive);
        }
        else // GameState::Playing
        {
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
                input.lookStickX = rx;
                input.lookStickY = ry;

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

            // 5. Update Physics Simulation & Mech
            physics->Update(deltaTime);
            mech.Update(deltaTime, input, physics.get(), &audioManager);
            camera.Update(deltaTime, mech);

            // 5.5 Auto-respawn local mech if destroyed
            if (mech.IsDestroyed() && mech.GetRespawnTimer() <= 0.0f)
            {
                uint8_t mySlot = network.GetLocalPlayerId();
                XMFLOAT3 spawnPos = { 0.0f, 2.0f, -40.0f };
                float spawnYaw = 0.0f;
                if (mySlot == 1) { spawnPos = { 0.0f, 2.0f, 40.0f }; spawnYaw = 3.14159265f; }
                else if (mySlot == 2) { spawnPos = { 40.0f, 2.0f, 0.0f }; spawnYaw = -1.5707963f; }
                else if (mySlot == 3) { spawnPos = { -40.0f, 2.0f, 0.0f }; spawnYaw = 1.5707963f; }
                mech.Respawn(spawnPos, spawnYaw);
            }

            // 5.6 Update Network Manager (Send 60Hz local state, receive remote packets, update RemoteMechs)
            network.Update(deltaTime, mech, &weapons, &audioManager, physics.get());
            if (network.ConsumeArenaModeChanged())
            {
                setArenaMode(static_cast<ArenaMode>(network.GetSyncedArenaMode()), false);
            }

            // 5.7 Update Autonomous AI Combat Mechs (if in CombatBots arena mode)
            if (currentArenaMode == ArenaMode::CombatBots)
            {
                for (auto& bot : aiBots)
                {
                    bot.Update(deltaTime, physics.get(), mech.GetPosition(), mech.GetVelocity(), !mech.IsDestroyed(), &audioManager, &weapons, &network.GetRemoteMechs());
                }
            }

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

            // 7. Update Target Lock System (FCS Soft-Lock & Target Assist Hard-Lock with RemoteMechs and AIBots support)
            auto& curTargets = (currentArenaMode == ArenaMode::TargetDummies) ? targets : emptyTargets;
            auto* curBots = (currentArenaMode == ArenaMode::CombatBots) ? &aiBots : nullptr;

            if (vrActive && vrManager->HasValidTracking())
            {
                XMMATRIX hmdViewProj = XMMatrixMultiply(vrManager->GetHmdView(), vrManager->GetHmdProj());
                XMFLOAT3 hmdPos = vrManager->GetHmdPosition();
                targetLock.Update(deltaTime, camera, mech, curTargets, input.yawDelta, input.pitchDelta, &audioManager, &hmdViewProj, &hmdPos, &network.GetRemoteMechs(), input.lookStickX, input.lookStickY, curBots);
            }
            else
            {
                targetLock.Update(deltaTime, camera, mech, curTargets, input.yawDelta, input.pitchDelta, &audioManager, nullptr, nullptr, &network.GetRemoteMechs(), input.lookStickX, input.lookStickY, curBots);
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

            const auto& lockInfo = targetLock.GetCurrentTarget();
            const XMFLOAT3* lockPosPtr = lockInfo.hasTarget ? &lockInfo.worldPos : nullptr;

            // Compute robotic arm aiming elevation (Aim Pitch) towards aimTarget with smooth servo interpolation
            XMFLOAT3 mechPos = mech.GetPosition();
            float dx = aimTarget.x - mechPos.x;
            float dy = aimTarget.y - (mechPos.y + 1.2f); // Weapon mount height (~1.2m)
            float dz = aimTarget.z - mechPos.z;
            float horizDist = std::sqrt(dx * dx + dz * dz);
            float targetAimPitch = 0.0f;
            if (horizDist > 0.5f)
            {
                targetAimPitch = std::atan2(dy, horizDist);
                // Clamp arm articulation pitch limit (-45 deg to +45 deg)
                targetAimPitch = std::clamp(targetAimPitch, -0.785f, 0.785f);
            }
            float curAimPitch = mech.GetAimPitch();
            float newAimPitch = curAimPitch + (targetAimPitch - curAimPitch) * std::min(1.0f, deltaTime * 12.0f);
            mech.SetAimPitch(newAimPitch);

            if (fireRightRequested)
            {
                float kick = weapons.GetSlot(Overdrive::WeaponSlot::RightArm).data.recoilKick;
                if (weapons.FireRightArm(mech.GetRightMuzzlePosition(), aimTarget, &audioManager, lockPosPtr))
                {
                    mech.TriggerRecoilRight(kick);
                    network.SendFireEvent(false, mech.GetRightMuzzlePosition(), aimTarget);
                }
            }
            if (fireLeftRequested)
            {
                float kick = weapons.GetSlot(Overdrive::WeaponSlot::LeftArm).data.recoilKick;
                if (weapons.FireLeftArm(mech.GetLeftMuzzlePosition(), aimTarget, &audioManager, lockPosPtr))
                {
                    mech.TriggerRecoilLeft(kick);
                    network.SendFireEvent(true, mech.GetLeftMuzzlePosition(), aimTarget);
                }
            }
            if (fireRightShoulderRequested)
            {
                auto res = weapons.TriggerRightShoulder(mech.GetRightBackMuzzlePosition(), aimTarget, &audioManager, lockPosPtr);
                if (res == Overdrive::ShoulderResult::Fired)
                {
                    network.SendFireEvent(false, mech.GetRightBackMuzzlePosition(), aimTarget);
                }
            }
            if (fireLeftShoulderRequested)
            {
                auto res = weapons.TriggerLeftShoulder(mech.GetLeftBackMuzzlePosition(), aimTarget, &audioManager, lockPosPtr);
                if (res == Overdrive::ShoulderResult::Fired)
                {
                    network.SendFireEvent(true, mech.GetLeftBackMuzzlePosition(), aimTarget);
                }
            }

            // 8.5 Boost Kick Collision Detection & Impact Delivery
            if (mech.IsBoostKicking() && !mech.HasBoostKickHit())
            {
                float kickDmg = frameSystem.GetKickDamage();
                float kickImpact = frameSystem.GetKickImpact();
                float kickDirectHitMult = 2.0f;

                XMFLOAT3 myPos = mech.GetPosition();
                float sinY = std::sin(mech.GetYaw());
                float cosY = std::cos(mech.GetYaw());
                XMFLOAT3 kickFwd = { sinY, 0.0f, cosY };

                // Kick hit center is positioned slightly ahead of the mech (~2.2m)
                XMFLOAT3 kickCenter = {
                    myPos.x + kickFwd.x * 2.2f,
                    myPos.y + 0.8f,
                    myPos.z + kickFwd.z * 2.2f
                };
                float hitRadius = 2.4f;

                // (A) Check against Target Dummies
                for (auto& dummy : targets)
                {
                    if (!dummy.IsAlive()) continue;
                    XMFLOAT3 dPos = dummy.GetPosition();
                    float dx = dPos.x - kickCenter.x;
                    float dy = dPos.y - kickCenter.y;
                    float dz = dPos.z - kickCenter.z;
                    float distSq = dx * dx + dy * dy + dz * dz;

                    if (distSq <= hitRadius * hitRadius)
                    {
                        dummy.TakeDamage(kickDmg, kickImpact, kickDirectHitMult, &audioManager);
                        audioManager.PlayBoostKickHit(dPos);
                        mech.SetBoostKickHit(true);

                        std::cout << "[COMBAT] >> BOOST KICK HIT ON DUMMY! "
                                  << dummy.GetName() << " (Damage: " << static_cast<int>(kickDmg)
                                  << ", Impact: " << static_cast<int>(kickImpact) << ") <<" << std::endl;
                        break;
                    }
                }

                // (B) Check against Remote Mechs (Multiplayer)
                if (!mech.HasBoostKickHit())
                {
                    auto& remoteMechs = network.GetRemoteMechs();
                    for (auto& rMech : remoteMechs)
                    {
                        if (!rMech.IsAlive() || rMech.GetPlayerId() == network.GetLocalPlayerId()) continue;
                        XMFLOAT3 rPos = rMech.GetPosition();
                        float dx = rPos.x - kickCenter.x;
                        float dy = rPos.y - kickCenter.y;
                        float dz = rPos.z - kickCenter.z;
                        float distSq = dx * dx + dy * dy + dz * dz;

                        if (distSq <= hitRadius * hitRadius)
                        {
                            rMech.TakeDamage(kickDmg, kickImpact, kickDirectHitMult, &audioManager);
                            rMech.ApplyKnockback(kickFwd, 32.0f); // Launch target backward
                            network.SendHitEvent(rMech.GetPlayerId(), kickDmg, rPos, kickImpact, kickDirectHitMult);
                            audioManager.PlayBoostKickHit(rPos);
                            mech.SetBoostKickHit(true);

                            std::cout << "[COMBAT] >> BOOST KICK HIT ON REMOTE PLAYER "
                                      << static_cast<int>(rMech.GetPlayerId() + 1)
                                      << "! (Damage: " << static_cast<int>(kickDmg)
                                      << ", Impact: " << static_cast<int>(kickImpact) << ") <<" << std::endl;
                            break;
                        }
                    }
                }

                // (C) Check against Autonomous AI Combat Bots
                if (!mech.HasBoostKickHit() && currentArenaMode == ArenaMode::CombatBots)
                {
                    for (auto& bot : aiBots)
                    {
                        if (!bot.IsAlive()) continue;
                        XMFLOAT3 bPos = bot.GetPosition();
                        float dx = bPos.x - kickCenter.x;
                        float dy = bPos.y - kickCenter.y;
                        float dz = bPos.z - kickCenter.z;
                        float distSq = dx * dx + dy * dy + dz * dz;

                        if (distSq <= hitRadius * hitRadius)
                        {
                            bot.TakeDamage(kickDmg, kickImpact, kickDirectHitMult, &audioManager);
                            bot.ApplyKnockback(kickFwd, 32.0f);
                            audioManager.PlayBoostKickHit(bPos);
                            mech.SetBoostKickHit(true);

                            std::cout << "[COMBAT] >> BOOST KICK HIT ON AI BOT "
                                      << static_cast<int>(bot.GetBotId() + 1)
                                      << "! (Damage: " << static_cast<int>(kickDmg)
                                      << ", Impact: " << static_cast<int>(kickImpact) << ") <<" << std::endl;
                            break;
                        }
                    }
                }
            }

            // 9. Update Weapons, Projectiles & Targets (Raycasts against remote mechs, dummies, and AI bots)
            weapons.Update(deltaTime, physics.get(), curTargets, &audioManager, &network.GetRemoteMechs(), &network, &mech, curBots);
            if (currentArenaMode == ArenaMode::TargetDummies)
            {
                for (auto& target : targets)
                {
                    target.Update(deltaTime, physics.get());
                }
            }

            // 10. Render Frame (Stereo VR if HMD active, and Desktop window mirror)
            if (vrActive)
            {
                vrManager->RenderFrame(renderer.get(), mech, weapons, curTargets, targetLock, &network.GetRemoteMechs(), &network, &frameSystem, curBots);
            }

            // Render to Desktop Window (Mirror view)
            // When VR is active, disable desktop VSync to avoid competing with HMD display refresh
            renderer->BeginFrame();
            renderer->RenderScene(camera, mech, weapons, curTargets, targetLock, &network.GetRemoteMechs(), &network, &frameSystem, curBots);
            renderer->EndFrame(!vrActive);
        }
    }

    // Save final assembled mech state
    Overdrive::LoadoutManager::SaveLoadout("loadout.ini", weapons, frameSystem, internalSystem);

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
