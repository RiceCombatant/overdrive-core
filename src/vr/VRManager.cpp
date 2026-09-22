#include "vr/VRManager.hpp"
#include "core/MechController.hpp"
#include "graphics/D3D11Renderer.hpp"
#include "combat/WeaponSystem.hpp"
#include "combat/TargetDummy.hpp"
#include "combat/TargetLockSystem.hpp"
#include <iostream>
#include <cmath>
#include <cstring>

namespace Overdrive
{
    VRManager::VRManager()
    {
    }

    VRManager::~VRManager()
    {
        Shutdown();
    }

    bool VRManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
    {
        if (!device || !context) return false;

        std::cout << "[VR] Checking for OpenXR Runtime (Meta Quest / SteamVR)..." << std::endl;

        if (!CreateInstance())
        {
            std::cout << "[VR] OpenXR runtime not available. Running in standard Desktop mode." << std::endl;
            m_isAvailable = false;
            return false;
        }

        if (!GetSystem())
        {
            std::cout << "[VR] No VR Headset (HMD) detected. Running in standard Desktop mode." << std::endl;
            Shutdown();
            return false;
        }

        if (!CreateSession(device))
        {
            std::cout << "[VR] Failed to create OpenXR session. Running in standard Desktop mode." << std::endl;
            Shutdown();
            return false;
        }

        if (!CreateSwapchains(device))
        {
            std::cout << "[VR] Failed to create OpenXR swapchains. Running in standard Desktop mode." << std::endl;
            Shutdown();
            return false;
        }

        m_isAvailable = true;
        std::cout << "========================================================" << std::endl;
        std::cout << " [VR] Meta Quest / OpenXR PCVR System Online!           " << std::endl;
        std::cout << " [VR] 6DoF Cockpit Stereo Rendering & Head Tracking OK! " << std::endl;
        std::cout << "========================================================" << std::endl;
        return true;
    }

    void VRManager::Shutdown()
    {
        for (int i = 0; i < 2; ++i)
        {
            m_eyes[i].rtvs.clear();
            m_eyes[i].dsv.Reset();
            m_eyes[i].depthTexture.Reset();
            if (m_eyes[i].swapchain != XR_NULL_HANDLE)
            {
                xrDestroySwapchain(m_eyes[i].swapchain);
                m_eyes[i].swapchain = XR_NULL_HANDLE;
            }
        }

        if (m_appSpace != XR_NULL_HANDLE)
        {
            xrDestroySpace(m_appSpace);
            m_appSpace = XR_NULL_HANDLE;
        }

        if (m_session != XR_NULL_HANDLE)
        {
            xrDestroySession(m_session);
            m_session = XR_NULL_HANDLE;
        }

        if (m_instance != XR_NULL_HANDLE)
        {
            xrDestroyInstance(m_instance);
            m_instance = XR_NULL_HANDLE;
        }

        m_isAvailable = false;
        m_isSessionRunning = false;
    }

    bool VRManager::CreateInstance()
    {
        uint32_t extCount = 0;
        xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);
        std::vector<XrExtensionProperties> exts(extCount, { XR_TYPE_EXTENSION_PROPERTIES });
        xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, exts.data());

        bool d3d11Supported = false;
        for (const auto& ext : exts)
        {
            if (strcmp(ext.extensionName, XR_KHR_D3D11_ENABLE_EXTENSION_NAME) == 0)
            {
                d3d11Supported = true;
                break;
            }
        }

        if (!d3d11Supported)
        {
            std::cout << "[VR] OpenXR D3D11 extension (" << XR_KHR_D3D11_ENABLE_EXTENSION_NAME << ") not supported by runtime." << std::endl;
            return false;
        }

        const char* enabledExtensions[] = {
            XR_KHR_D3D11_ENABLE_EXTENSION_NAME
        };

        XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
        strcpy_s(createInfo.applicationInfo.applicationName, "Overdrive Core VR");
        createInfo.applicationInfo.applicationVersion = 1;
        strcpy_s(createInfo.applicationInfo.engineName, "Overdrive Core Engine");
        createInfo.applicationInfo.engineVersion = 1;
        createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

        createInfo.enabledExtensionCount = 1;
        createInfo.enabledExtensionNames = enabledExtensions;

        XrResult res = xrCreateInstance(&createInfo, &m_instance);
        return (res == XR_SUCCESS);
    }

    bool VRManager::GetSystem()
    {
        XrSystemGetInfo sysInfo = { XR_TYPE_SYSTEM_GET_INFO };
        sysInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

        XrResult res = xrGetSystem(m_instance, &sysInfo, &m_systemId);
        if (res != XR_SUCCESS) return false;

        XrSystemProperties sysProps = { XR_TYPE_SYSTEM_PROPERTIES };
        xrGetSystemProperties(m_instance, m_systemId, &sysProps);
        std::cout << "[VR] Connected HMD: " << sysProps.systemName << std::endl;

        return true;
    }

    bool VRManager::CreateSession(ID3D11Device* device)
    {
        // Get D3D11 Graphics Requirements
        PFN_xrGetD3D11GraphicsRequirementsKHR pfnGetD3D11Reqs = nullptr;
        xrGetInstanceProcAddr(
            m_instance,
            "xrGetD3D11GraphicsRequirementsKHR",
            reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetD3D11Reqs)
        );

        if (pfnGetD3D11Reqs)
        {
            XrGraphicsRequirementsD3D11KHR graphicsReqs = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
            pfnGetD3D11Reqs(m_instance, m_systemId, &graphicsReqs);
        }

        XrGraphicsBindingD3D11KHR d3dBinding = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
        d3dBinding.device = device;

        XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
        sessionInfo.next = &d3dBinding;
        sessionInfo.systemId = m_systemId;

        XrResult res = xrCreateSession(m_instance, &sessionInfo, &m_session);
        if (res != XR_SUCCESS) return false;

        // Create Reference Space (LOCAL for seated mech cockpit experience)
        XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        spaceInfo.poseInReferenceSpace.orientation = { 0, 0, 0, 1 };
        spaceInfo.poseInReferenceSpace.position = { 0, 0, 0 };

        res = xrCreateReferenceSpace(m_session, &spaceInfo, &m_appSpace);
        return (res == XR_SUCCESS);
    }

    bool VRManager::CreateSwapchains(ID3D11Device* device)
    {
        uint32_t viewCount = 0;
        xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
        m_configViews.resize(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
        xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, m_configViews.data());

        if (viewCount < 2) return false;

        m_views.resize(viewCount, { XR_TYPE_VIEW });

        for (int i = 0; i < 2; ++i)
        {
            auto& eye = m_eyes[i];
            eye.width  = m_configViews[i].recommendedImageRectWidth;
            eye.height = m_configViews[i].recommendedImageRectHeight;

            std::cout << "[VR] Eye " << (i == 0 ? "Left" : "Right")
                      << " Recommended Resolution: " << eye.width << "x" << eye.height << std::endl;

            XrSwapchainCreateInfo scInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
            scInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
            scInfo.format = DXGI_FORMAT_R8G8B8A8_UNORM;
            scInfo.sampleCount = 1;
            scInfo.width = eye.width;
            scInfo.height = eye.height;
            scInfo.faceCount = 1;
            scInfo.arraySize = 1;
            scInfo.mipCount = 1;

            XrResult res = xrCreateSwapchain(m_session, &scInfo, &eye.swapchain);
            if (res != XR_SUCCESS) return false;

            uint32_t imageCount = 0;
            xrEnumerateSwapchainImages(eye.swapchain, 0, &imageCount, nullptr);
            eye.colorImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
            xrEnumerateSwapchainImages(eye.swapchain, imageCount, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(eye.colorImages.data()));

            // Create Render Target Views for each swapchain image
            eye.rtvs.resize(imageCount);
            for (uint32_t j = 0; j < imageCount; ++j)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
                rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

                HRESULT hr = device->CreateRenderTargetView(eye.colorImages[j].texture, &rtvDesc, eye.rtvs[j].GetAddressOf());
                if (FAILED(hr)) return false;
            }

            // Create Depth Stencil Buffer for this eye
            D3D11_TEXTURE2D_DESC depthDesc = {};
            depthDesc.Width = eye.width;
            depthDesc.Height = eye.height;
            depthDesc.MipLevels = 1;
            depthDesc.ArraySize = 1;
            depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            depthDesc.SampleDesc.Count = 1;
            depthDesc.Usage = D3D11_USAGE_DEFAULT;
            depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

            HRESULT hr = device->CreateTexture2D(&depthDesc, nullptr, eye.depthTexture.GetAddressOf());
            if (FAILED(hr)) return false;

            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = depthDesc.Format;
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

            hr = device->CreateDepthStencilView(eye.depthTexture.Get(), &dsvDesc, eye.dsv.GetAddressOf());
            if (FAILED(hr)) return false;

            // Viewport
            eye.viewport.TopLeftX = 0.0f;
            eye.viewport.TopLeftY = 0.0f;
            eye.viewport.Width = static_cast<float>(eye.width);
            eye.viewport.Height = static_cast<float>(eye.height);
            eye.viewport.MinDepth = 0.0f;
            eye.viewport.MaxDepth = 1.0f;
        }

        return true;
    }

    void VRManager::PollEvents()
    {
        if (!m_isAvailable || m_instance == XR_NULL_HANDLE) return;

        XrEventDataBuffer eventData = { XR_TYPE_EVENT_DATA_BUFFER };
        while (xrPollEvent(m_instance, &eventData) == XR_SUCCESS)
        {
            if (eventData.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
            {
                auto* stateChanged = reinterpret_cast<XrEventDataSessionStateChanged*>(&eventData);
                m_sessionState = stateChanged->state;

                if (m_sessionState == XR_SESSION_STATE_READY)
                {
                    XrSessionBeginInfo beginInfo = { XR_TYPE_SESSION_BEGIN_INFO };
                    beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    xrBeginSession(m_session, &beginInfo);
                    m_isSessionRunning = true;
                    std::cout << "[VR] Session started & rendering enabled." << std::endl;
                }
                else if (m_sessionState == XR_SESSION_STATE_STOPPING)
                {
                    xrEndSession(m_session);
                    m_isSessionRunning = false;
                    std::cout << "[VR] Session stopped." << std::endl;
                }
            }

            eventData = { XR_TYPE_EVENT_DATA_BUFFER };
        }
    }

    XMMATRIX VRManager::CreateProjectionFromFov(const XrFovf& fov, float nearZ, float farZ)
    {
        float tanLeft  = std::tan(fov.angleLeft);
        float tanRight = std::tan(fov.angleRight);
        float tanDown  = std::tan(fov.angleDown);
        float tanUp    = std::tan(fov.angleUp);

        float tanWidth  = tanRight - tanLeft;
        float tanHeight = tanUp - tanDown;

        // DirectX 11 Left-Handed projection matrix (Z: 0 to 1)
        XMMATRIX mat = XMMatrixSet(
            2.0f / tanWidth, 0.0f, 0.0f, 0.0f,
            0.0f, 2.0f / tanHeight, 0.0f, 0.0f,
            -(tanRight + tanLeft) / tanWidth, -(tanUp + tanDown) / tanHeight, farZ / (farZ - nearZ), 1.0f,
            0.0f, 0.0f, -(farZ * nearZ) / (farZ - nearZ), 0.0f
        );

        return mat;
    }

    bool VRManager::BeginFrame(const MechController& mech)
    {
        if (!m_isAvailable || !m_isSessionRunning) return false;

        XrFrameWaitInfo waitInfo = { XR_TYPE_FRAME_WAIT_INFO };
        XrResult res = xrWaitFrame(m_session, &waitInfo, &m_frameState);
        if (res != XR_SUCCESS) return false;

        XrFrameBeginInfo beginInfo = { XR_TYPE_FRAME_BEGIN_INFO };
        res = xrBeginFrame(m_session, &beginInfo);
        if (res != XR_SUCCESS) return false;

        if (!m_frameState.shouldRender) return false;

        // Locate Views (6DoF Head tracking for Left and Right Eye)
        XrViewState viewState = { XR_TYPE_VIEW_STATE };
        XrViewLocateInfo locateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
        locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        locateInfo.displayTime = m_frameState.predictedDisplayTime;
        locateInfo.space = m_appSpace;

        uint32_t viewCount = 2;
        res = xrLocateViews(m_session, &locateInfo, &viewState, 2, &viewCount, m_views.data());
        if (res != XR_SUCCESS || viewCount < 2) return false;

        // Base mech cockpit head position and Yaw
        XMFLOAT3 cockpitPos = mech.GetCockpitHeadPosition();
        float mechYaw = mech.GetYaw();
        XMVECTOR quatMechYaw = XMQuaternionRotationRollPitchYaw(0.0f, mechYaw, 0.0f);
        XMMATRIX rotMechY = XMMatrixRotationY(mechYaw);

        for (int i = 0; i < 2; ++i)
        {
            // Acquire swapchain image for this eye
            XrSwapchainImageAcquireInfo acqInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
            xrAcquireSwapchainImage(m_eyes[i].swapchain, &acqInfo, &m_currentImageIndices[i]);

            XrSwapchainImageWaitInfo waitImageInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
            waitImageInfo.timeout = XR_INFINITE_DURATION;
            xrWaitSwapchainImage(m_eyes[i].swapchain, &waitImageInfo);

            // Compute World-Space Eye Transform
            // Invert OpenXR Z to match DirectX left-handed coordinate convention
            XrPosef eyePose = m_views[i].pose;
            XMVECTOR hmdLocalPos = XMVectorSet(eyePose.position.x, eyePose.position.y, -eyePose.position.z, 0.0f);
            XMVECTOR hmdLocalRot = XMVectorSet(-eyePose.orientation.x, -eyePose.orientation.y, eyePose.orientation.z, eyePose.orientation.w);

            // Compose with Mech world position and Yaw
            XMVECTOR eyeWorldPos = XMVector3TransformCoord(hmdLocalPos, rotMechY) + XMLoadFloat3(&cockpitPos);
            XMVECTOR eyeWorldRot = XMQuaternionMultiply(hmdLocalRot, quatMechYaw);

            // Calculate View Matrix
            XMMATRIX worldMatrix = XMMatrixRotationQuaternion(eyeWorldRot) * XMMatrixTranslationFromVector(eyeWorldPos);
            m_eyeView[i] = XMMatrixInverse(nullptr, worldMatrix);

            // Calculate Projection Matrix from Eye FOV
            m_eyeProj[i] = CreateProjectionFromFov(m_views[i].fov, 0.05f, 1000.0f);
        }

        return true;
    }

    void VRManager::RenderStereo(
        D3D11Renderer* renderer,
        const MechController& mech,
        const WeaponSystem& weapons,
        const std::vector<TargetDummy>& targets,
        const TargetLockSystem& lockSystem)
    {
        if (!m_isAvailable || !m_isSessionRunning || !renderer) return;

        for (int i = 0; i < 2; ++i)
        {
            auto& eye = m_eyes[i];
            ID3D11RenderTargetView* rtv = eye.rtvs[m_currentImageIndices[i]].Get();
            ID3D11DepthStencilView* dsv = eye.dsv.Get();

            renderer->RenderVREye(
                m_eyeView[i],
                m_eyeProj[i],
                rtv,
                dsv,
                eye.viewport,
                mech,
                weapons,
                targets,
                lockSystem,
                (i == 0) // isLeftEye
            );
        }
    }

    void VRManager::EndFrame()
    {
        if (!m_isAvailable || !m_isSessionRunning) return;

        for (int i = 0; i < 2; ++i)
        {
            XrSwapchainImageReleaseInfo relInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
            xrReleaseSwapchainImage(m_eyes[i].swapchain, &relInfo);
        }

        XrCompositionLayerProjectionView projViews[2] = {
            { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW },
            { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW }
        };

        for (int i = 0; i < 2; ++i)
        {
            projViews[i].pose = m_views[i].pose;
            projViews[i].fov  = m_views[i].fov;
            projViews[i].subImage.swapchain = m_eyes[i].swapchain;
            projViews[i].subImage.imageRect.offset = { 0, 0 };
            projViews[i].subImage.imageRect.extent = {
                static_cast<int32_t>(m_eyes[i].width),
                static_cast<int32_t>(m_eyes[i].height)
            };
            projViews[i].subImage.imageArrayIndex = 0;
        }

        XrCompositionLayerProjection projLayer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        projLayer.space = m_appSpace;
        projLayer.viewCount = 2;
        projLayer.views = projViews;

        const XrCompositionLayerBaseHeader* layers[] = {
            reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projLayer)
        };

        XrFrameEndInfo endInfo = { XR_TYPE_FRAME_END_INFO };
        endInfo.displayTime = m_frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = 1;
        endInfo.layers = layers;

        xrEndFrame(m_session, &endInfo);
    }
}
