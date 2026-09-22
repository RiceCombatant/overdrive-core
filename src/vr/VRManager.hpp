#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <string>

#ifndef XR_USE_PLATFORM_WIN32
#define XR_USE_PLATFORM_WIN32
#endif

#ifndef XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D11
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

namespace Overdrive
{
    using namespace DirectX;
    using Microsoft::WRL::ComPtr;

    class MechController;
    class WeaponSystem;
    class TargetDummy;
    class TargetLockSystem;
    class D3D11Renderer;
    class RemoteMech;
    class NetworkManager;

    struct VREyeData
    {
        XrSwapchain swapchain = XR_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<XrSwapchainImageD3D11KHR> colorImages;
        std::vector<ComPtr<ID3D11RenderTargetView>> rtvs;
        ComPtr<ID3D11Texture2D> depthTexture;
        ComPtr<ID3D11DepthStencilView> dsv;
        D3D11_VIEWPORT viewport = {};
    };

    class VRManager
    {
    public:
        VRManager();
        ~VRManager();

        // Probes OpenXR runtime and returns required GPU Adapter LUID if available
        bool PreInitialize(LUID* outLuid = nullptr);

        // Initialize OpenXR runtime with D3D11 device. Returns false if no HMD/runtime is available.
        bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
        void Shutdown();

        bool IsAvailable() const { return m_isAvailable; }
        bool IsSessionRunning() const { return m_isSessionRunning; }
        LUID GetRequiredLuid() const { return m_requiredLuid; }

        // Poll OpenXR events (session state transitions, focus, pause)
        void PollEvents();

        // Frame cycle: handles xrWaitFrame, xrBeginFrame, eye rendering, and xrEndFrame atomically
        bool RenderFrame(
            D3D11Renderer* renderer,
            const MechController& mech,
            const WeaponSystem& weapons,
            const std::vector<TargetDummy>& targets,
            const TargetLockSystem& lockSystem,
            const std::vector<RemoteMech>* remoteMechs = nullptr,
            const NetworkManager* network = nullptr
        );

        // Eye matrices (world space view & projection for left/right eye)
        XMMATRIX GetEyeView(int eye) const { return m_eyeView[eye]; }
        XMMATRIX GetEyeProj(int eye) const { return m_eyeProj[eye]; }

        // Center head pose in world space (for HMD gaze targeting)
        XMFLOAT3 GetHmdPosition() const { return m_hmdWorldPos; }
        XMFLOAT3 GetHmdForward() const { return m_hmdWorldForward; }
        XMMATRIX GetHmdView() const { return m_hmdView; }
        XMMATRIX GetHmdProj() const { return m_hmdProj; }
        bool HasValidTracking() const { return m_trackingValid; }

    private:
        bool CreateInstance();
        bool GetSystem();
        bool CreateSession(ID3D11Device* device);
        bool CreateSwapchains(ID3D11Device* device);
        DXGI_FORMAT SelectSwapchainFormat(const std::vector<int64_t>& runtimeFormats);

        XMMATRIX CreateProjectionFromFov(const XrFovf& fov, float nearZ = 0.05f, float farZ = 1000.0f);

        bool m_isAvailable = false;
        bool m_isSessionRunning = false;
        LUID m_requiredLuid = {};

        XrInstance m_instance = XR_NULL_HANDLE;
        XrSystemId m_systemId = XR_NULL_SYSTEM_ID;
        XrSession m_session = XR_NULL_HANDLE;
        XrSpace m_appSpace = XR_NULL_HANDLE;
        XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;

        XrFrameState m_frameState = { XR_TYPE_FRAME_STATE };
        std::vector<XrView> m_views;
        std::vector<XrViewConfigurationView> m_configViews;

        VREyeData m_eyes[2]; // 0: Left, 1: Right
        XMMATRIX m_eyeView[2];
        XMMATRIX m_eyeProj[2];
        uint32_t m_currentImageIndices[2] = { 0, 0 };

        bool m_trackingValid = false;
        XMFLOAT3 m_hmdWorldPos = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_hmdWorldForward = { 0.0f, 0.0f, 1.0f };
        XMMATRIX m_hmdView = XMMatrixIdentity();
        XMMATRIX m_hmdProj = XMMatrixIdentity();

        // Desktop Mirror
        ComPtr<ID3D11ShaderResourceView> m_mirrorSRV;
    };
}
