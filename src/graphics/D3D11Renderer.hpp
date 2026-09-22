#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include "math/MathTypes.hpp"
#include "graphics/Camera.hpp"
#include "graphics/GridFloor.hpp"
#include "core/MechController.hpp"
#include "combat/WeaponSystem.hpp"
#include "combat/TargetDummy.hpp"

using Microsoft::WRL::ComPtr;

namespace Overdrive
{
    class GridFloor;
    class TargetDummy;
    class TargetLockSystem;

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
            const TargetLockSystem& lockSystem
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
            bool isLeftEye
        );

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
        bool InitHUDGeometry();
        bool InitCombatGeometry();

        void CleanupRenderTarget();

        void RenderMech(const MechController& mech, const XMMATRIX& view, const XMMATRIX& proj, bool isFPV);
        void RenderProjectiles(const WeaponSystem& weapons, const XMMATRIX& view, const XMMATRIX& proj);
        void RenderTargetDummies(const std::vector<TargetDummy>& targets, const XMMATRIX& view, const XMMATRIX& proj);
        void RenderHUD(const MechController& mech, const WeaponSystem& weapons, const TargetLockSystem& lockSystem, CameraMode cameraMode);
        void RenderVRHUD(const MechController& mech, const WeaponSystem& weapons, const TargetLockSystem& lockSystem, const XMMATRIX& view, const XMMATRIX& proj);

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

        ComPtr<ID3D11VertexShader> m_vertexShader;
        ComPtr<ID3D11PixelShader> m_pixelShader;
        ComPtr<ID3D11InputLayout> m_inputLayout;
        ComPtr<ID3D11Buffer> m_constantBuffer;

        // Scene elements
        GridFloor m_gridFloor;

        // Mech geometry
        ComPtr<ID3D11Buffer> m_mechVertexBuffer;
        ComPtr<ID3D11Buffer> m_mechIndexBuffer;
        UINT m_mechIndexCount = 0;

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
    };
}
