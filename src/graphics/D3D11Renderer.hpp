#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "math/MathTypes.hpp"
#include "graphics/Camera.hpp"
#include "graphics/GridFloor.hpp"
#include "graphics/ModelLoader.hpp"
#include "core/MechController.hpp"
#include "core/FrameSystem.hpp"
#include "combat/WeaponSystem.hpp"
#include "combat/TargetDummy.hpp"

using Microsoft::WRL::ComPtr;

namespace Overdrive
{
    class GridFloor;
    class TargetDummy;
    class TargetLockSystem;
    class RemoteMech;
    class AIBotMech;
    class NetworkManager;
    class MenuSystem;
    class InternalSystem;

    enum class AssembleTab
    {
        Weapons  = 0,
        Frame    = 1,
        Internal = 2
    };

    class D3D11Renderer
    {
    public:
        D3D11Renderer();
        ~D3D11Renderer();

        bool Initialize(HWND hwnd, int width, int height, const LUID* preferredLuid = nullptr);
        void Resize(int width, int height);
        void BeginFrame();
        void EndFrame(bool vsync = true);

        void RenderScene(
            const Camera& camera,
            const MechController& mech,
            const WeaponSystem& weapons,
            const std::vector<TargetDummy>& targets,
            const TargetLockSystem& lockSystem,
            const std::vector<RemoteMech>* remoteMechs = nullptr,
            const NetworkManager* network = nullptr,
            const FrameSystem* frames = nullptr,
            const std::vector<AIBotMech>* aiBots = nullptr
        );

        // Main Menu rendering
        void RenderMainMenu(
            const MenuSystem& menu,
            const Camera& camera,
            const MechController& mech
        );

        // Garage / Assemble rendering
        void RenderAssembleMenu(
            const Camera& camera,
            const MechController& mech,
            const WeaponSystem& weapons,
            const FrameSystem& frames,
            const InternalSystem& internal,
            AssembleTab currentTab,
            int selectedSlotIndex,
            float animTime
        );

        void RenderVRMainMenu(
            const MenuSystem& menu,
            const XMMATRIX& view,
            const XMMATRIX& proj
        );

        // OpenXR Stereo VR Eye rendering
        void RenderVREye(
            const XMMATRIX& view,
            const XMMATRIX& proj,
            ID3D11RenderTargetView* rtv,
            ID3D11DepthStencilView* dsv,
            const D3D11_VIEWPORT& viewport,
            const MechController& mech,
            const WeaponSystem& weapons,
            const std::vector<TargetDummy>& targets,
            const TargetLockSystem& lockSystem,
            bool isLeftEye,
            const std::vector<RemoteMech>* remoteMechs = nullptr,
            const NetworkManager* network = nullptr,
            const FrameSystem* frames = nullptr,
            const std::vector<AIBotMech>* aiBots = nullptr
        );

        void ShowArenaModeBanner(const std::string& text, float duration = 2.5f);

        ID3D11Device* GetDevice() const { return m_device.Get(); }
        ID3D11DeviceContext* GetContext() const { return m_context.Get(); }

        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }
        GridFloor& GetGridFloor() { return m_gridFloor; }

    private:
        bool CreateDeviceAndSwapChain(HWND hwnd, const LUID* preferredLuid = nullptr);
        bool CreateRenderTargetAndDepthBuffer();
        bool InitShadersAndInputLayout();
        bool InitMechGeometry();
        bool InitFrameMeshes();
        bool InitWeaponMeshes();
        bool InitJetGeometry();
        bool InitHUDGeometry();
        bool InitCombatGeometry();

        void CleanupRenderTarget();

        void RenderMech(const MechController& mech, const WeaponSystem& weapons, const FrameSystem* frames, const XMMATRIX& view, const XMMATRIX& proj, bool isFPV);
        void RenderFramePart(const FrameInstance& part, FrameSlot slot, const XMMATRIX& mountWorld, const XMMATRIX& view, const XMMATRIX& proj, bool isFPV = false);
        void RenderMechWeapon(const WeaponInstance& weapon, const XMMATRIX& mountWorld, const XMMATRIX& view, const XMMATRIX& proj);
        void RenderJetPlume(const XMMATRIX& nozzleWorld, float length, float radius, const XMFLOAT4& outerColor, const XMFLOAT4& coreColor, const XMMATRIX& view, const XMMATRIX& proj);
        void RenderMechThrusters(const MechController& mech, const XMMATRIX& mechWorld, const XMMATRIX& view, const XMMATRIX& proj, bool isFPV);
        void RenderRemoteMechThrusters(const RemoteMech& remoteMech, const XMMATRIX& mechWorld, const XMMATRIX& view, const XMMATRIX& proj);
        void RenderEnemyMech(const RemoteMech& remoteMech, const XMMATRIX& view, const XMMATRIX& proj);
        void RenderAIBots(const std::vector<AIBotMech>& aiBots, const XMMATRIX& view, const XMMATRIX& proj);
        void RenderAIBotThrusters(const AIBotMech& bot, const XMMATRIX& botWorld, const XMMATRIX& view, const XMMATRIX& proj);
        void RenderProjectiles(const WeaponSystem& weapons, const XMMATRIX& view, const XMMATRIX& proj);
        void RenderTargetDummies(const std::vector<TargetDummy>& targets, const XMMATRIX& view, const XMMATRIX& proj);
        void RenderTargetReticles(const std::vector<TargetDummy>& targets, const TargetLockSystem& lockSystem, const XMMATRIX& view, const XMMATRIX& proj, const XMFLOAT3& eyePos, const std::vector<RemoteMech>* remoteMechs = nullptr, const std::vector<AIBotMech>* aiBots = nullptr);
        void RenderHUD(
            const MechController& mech,
            const WeaponSystem& weapons,
            const TargetLockSystem& lockSystem,
            CameraMode cameraMode,
            const NetworkManager* network = nullptr,
            const std::vector<RemoteMech>* remoteMechs = nullptr,
            const std::vector<TargetDummy>* targets = nullptr,
            const Camera* camera = nullptr,
            const std::vector<AIBotMech>* aiBots = nullptr
        );
        void RenderVRHUD(const MechController& mech, const WeaponSystem& weapons, const TargetLockSystem& lockSystem, const XMMATRIX& view, const XMMATRIX& proj, const NetworkManager* network = nullptr);

    private:
        HWND m_hwnd = nullptr;
        int m_width = 1280;
        int m_height = 720;

        ComPtr<ID3D11Device> m_device;
        ComPtr<ID3D11DeviceContext> m_context;
        ComPtr<IDXGISwapChain> m_swapChain;

        ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
        ComPtr<ID3D11DepthStencilView> m_depthStencilView;
        ComPtr<ID3D11RasterizerState> m_rasterizerState;
        ComPtr<ID3D11DepthStencilState> m_depthStencilState;
        ComPtr<ID3D11DepthStencilState> m_hudDepthDisabledState;
        ComPtr<ID3D11DepthStencilState> m_thrusterDepthState;
        ComPtr<ID3D11BlendState> m_additiveBlendState;

        ComPtr<ID3D11VertexShader> m_vertexShader;
        ComPtr<ID3D11PixelShader> m_pixelShader;
        ComPtr<ID3D11InputLayout> m_inputLayout;
        ComPtr<ID3D11Buffer> m_constantBuffer;

        // Scene elements
        GridFloor m_gridFloor;

        // Mech geometry
        ComPtr<ID3D11Buffer> m_mechVertexBuffer;
        ComPtr<ID3D11Buffer> m_playerMechVertexBuffer[4]; // 0: Crimson, 1: Cobalt, 2: Amber, 3: Emerald
        ComPtr<ID3D11Buffer> m_enemyMechVertexBuffer; // Legacy/fallback
        ComPtr<ID3D11Buffer> m_mechIndexBuffer;
        UINT m_mechIndexCount = 0;

        // Frame parts geometry for modular mech assemble
        struct FrameMesh
        {
            ComPtr<ID3D11Buffer> vertexBuffer;
            ComPtr<ID3D11Buffer> indexBuffer;
            UINT indexCount = 0;
        };
        std::unordered_map<FramePartType, FrameMesh> m_frameMeshes;

        // Weapon geometry for modular assemble / hanger swap
        struct WeaponMesh
        {
            ComPtr<ID3D11Buffer> vertexBuffer;
            ComPtr<ID3D11Buffer> indexBuffer;
            UINT indexCount = 0;
        };
        std::unordered_map<WeaponType, WeaponMesh> m_weaponMeshes;
        ModelManager m_modelManager;

        // Thruster Jet Plume geometry
        ComPtr<ID3D11Buffer> m_jetVertexBuffer;
        ComPtr<ID3D11Buffer> m_jetIndexBuffer;
        UINT m_jetIndexCount = 0;

        // HUD geometry
        ComPtr<ID3D11Buffer> m_hudVertexBuffer;
        UINT m_reticleVertexCount = 0;
        ComPtr<ID3D11Buffer> m_dynamicEnBuffer;
        ComPtr<ID3D11Buffer> m_dynamicAmmoBuffer;
        ComPtr<ID3D11Buffer> m_dynamicReticleBuffer; // Dynamic inner reticle & distance display

        // Combat geometry
        ComPtr<ID3D11Buffer> m_dynamicProjectileBuffer;
        ComPtr<ID3D11Buffer> m_dummyVertexBuffer;
        ComPtr<ID3D11Buffer> m_dummyIndexBuffer;
        UINT m_dummyIndexCount = 0;

        // Arena mode / system banner overlay
        std::string m_bannerText = "";
        float m_bannerTimer = 0.0f;
        float m_bannerMaxDuration = 2.5f;
    };
}
