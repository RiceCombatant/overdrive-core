#include "vr/VRManager.hpp"
#include "core/MechController.hpp"
#include "graphics/D3D11Renderer.hpp"
#include "combat/WeaponSystem.hpp"
#include "combat/TargetDummy.hpp"
#include "combat/TargetLockSystem.hpp"
#include "ui/MenuSystem.hpp"
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

    bool VRManager::PreInitialize(LUID* outLuid)
    {
        if (outLuid) { outLuid->LowPart = 0; outLuid->HighPart = 0; }

        std::cout << "[VR] Probing OpenXR Runtime (Meta Quest / SteamVR)..." << std::endl;

        if (!CreateInstance())
        {
            std::cout << "[VR] OpenXR runtime not available or not running." << std::endl;
            return false;
        }

        if (!GetSystem())
        {
            std::cout << "[VR] No VR Headset (HMD) detected by OpenXR runtime." << std::endl;
            Shutdown();
            return false;
        }

        // Query D3D11 Requirements to obtain requested GPU LUID
        PFN_xrGetD3D11GraphicsRequirementsKHR pfnGetD3D11Reqs = nullptr;
        xrGetInstanceProcAddr(
            m_instance,
            "xrGetD3D11GraphicsRequirementsKHR",
            reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetD3D11Reqs)
        );

        if (pfnGetD3D11Reqs)
        {
            XrGraphicsRequirementsD3D11KHR graphicsReqs = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
            XrResult res = pfnGetD3D11Reqs(m_instance, m_systemId, &graphicsReqs);
            if (res == XR_SUCCESS)
            {
                m_requiredLuid = graphicsReqs.adapterLuid;
                if (outLuid)
                {
                    *outLuid = graphicsReqs.adapterLuid;
                }
                std::cout << "[VR] OpenXR Runtime requires GPU Adapter LUID: " << graphicsReqs.adapterLuid.LowPart << ":" << graphicsReqs.adapterLuid.HighPart << std::endl;
            }
            else
            {
                std::cout << "[VR] Query D3D11 requirements returned XrResult: " << res << std::endl;
            }
        }

        return true;
    }

    bool VRManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
    {
        if (!device || !context) return false;

        // Ensure PreInitialize was executed
        if (m_instance == XR_NULL_HANDLE || m_systemId == XR_NULL_SYSTEM_ID)
        {
            if (!PreInitialize(nullptr))
            {
                return false;
            }
        }

        if (!CreateSession(device))
        {
            std::cout << "[VR] Failed to create OpenXR session with D3D11 device." << std::endl;
            Shutdown();
            return false;
        }

        if (!CreateSwapchains(device))
        {
            std::cout << "[VR] Failed to create OpenXR swapchains." << std::endl;
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
        if (res != XR_SUCCESS)
        {
            std::cout << "[VR ERROR] xrCreateSession failed with XrResult: " << res << std::endl;
            return false;
        }

        // Create Reference Space (LOCAL for seated mech cockpit experience)
        XrReferenceSpaceCreateInfo spaceInfo = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        spaceInfo.poseInReferenceSpace.orientation = { 0, 0, 0, 1 };
        spaceInfo.poseInReferenceSpace.position = { 0, 0, 0 };

        res = xrCreateReferenceSpace(m_session, &spaceInfo, &m_appSpace);
        if (res != XR_SUCCESS)
        {
            std::cout << "[VR ERROR] xrCreateReferenceSpace failed with XrResult: " << res << std::endl;
            return false;
        }
        return true;
    }

    DXGI_FORMAT VRManager::SelectSwapchainFormat(const std::vector<int64_t>& runtimeFormats)
    {
        const DXGI_FORMAT preferredFormats[] = {
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM
        };

        for (auto preferred : preferredFormats)
        {
            for (auto runtimeFormat : runtimeFormats)
            {
                if (runtimeFormat == static_cast<int64_t>(preferred))
                {
                    return preferred;
                }
            }
        }

        return DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    bool VRManager::CreateSwapchains(ID3D11Device* device)
    {
        uint32_t viewCount = 0;
        xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
        m_configViews.resize(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
        xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, m_configViews.data());

        if (viewCount < 2) return false;

        m_views.resize(viewCount, { XR_TYPE_VIEW });

        // Enumerate swapchain formats supported by runtime
        uint32_t formatCount = 0;
        xrEnumerateSwapchainFormats(m_session, 0, &formatCount, nullptr);
        std::vector<int64_t> formats(formatCount);
        xrEnumerateSwapchainFormats(m_session, formatCount, &formatCount, formats.data());

        DXGI_FORMAT colorFormat = SelectSwapchainFormat(formats);
        std::cout << "[VR] Selected Swapchain Color Format: " << colorFormat << std::endl;

        for (int i = 0; i < 2; ++i)
        {
            auto& eye = m_eyes[i];
            eye.width  = m_configViews[i].recommendedImageRectWidth;
            eye.height = m_configViews[i].recommendedImageRectHeight;

            std::cout << "[VR] Eye " << (i == 0 ? "Left" : "Right")
                      << " Resolution: " << eye.width << "x" << eye.height << std::endl;

            XrSwapchainCreateInfo scInfo = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
            scInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
            scInfo.format = colorFormat;
            scInfo.sampleCount = 1;
            scInfo.width = eye.width;
            scInfo.height = eye.height;
            scInfo.faceCount = 1;
            scInfo.arraySize = 1;
            scInfo.mipCount = 1;

            XrResult res = xrCreateSwapchain(m_session, &scInfo, &eye.swapchain);
            if (res != XR_SUCCESS)
            {
                std::cout << "[VR ERROR] xrCreateSwapchain for eye " << i << " failed with XrResult: " << res << std::endl;
                return false;
            }

            uint32_t imageCount = 0;
            xrEnumerateSwapchainImages(eye.swapchain, 0, &imageCount, nullptr);
            eye.colorImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR });
            xrEnumerateSwapchainImages(eye.swapchain, imageCount, &imageCount, reinterpret_cast<XrSwapchainImageBaseHeader*>(eye.colorImages.data()));

            // Create Render Target Views for each swapchain image
            eye.rtvs.resize(imageCount);
            for (uint32_t j = 0; j < imageCount; ++j)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
                rtvDesc.Format = colorFormat;
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

    bool VRManager::RenderFrame(
        D3D11Renderer* renderer,
        const MechController& mech,
        const WeaponSystem& weapons,
        const std::vector<TargetDummy>& targets,
        const TargetLockSystem& lockSystem,
        const std::vector<RemoteMech>* remoteMechs,
        const NetworkManager* network,
        const FrameSystem* frames,
        const std::vector<AIBotMech>* aiBots)
    {
        if (!m_isAvailable || !m_isSessionRunning || !renderer) return false;

        XrFrameWaitInfo waitInfo = { XR_TYPE_FRAME_WAIT_INFO };
        XrFrameState frameState = { XR_TYPE_FRAME_STATE };
        XrResult res = xrWaitFrame(m_session, &waitInfo, &frameState);
        if (res != XR_SUCCESS) return false;

        XrFrameBeginInfo beginInfo = { XR_TYPE_FRAME_BEGIN_INFO };
        res = xrBeginFrame(m_session, &beginInfo);
        if (res != XR_SUCCESS) return false;

        bool rendered = false;
        XrCompositionLayerProjection projLayer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        XrCompositionLayerProjectionView projViews[2] = {
            { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW },
            { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW }
        };

        if (frameState.shouldRender == XR_TRUE)
        {
            XrViewState viewState = { XR_TYPE_VIEW_STATE };
            XrViewLocateInfo locateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
            locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            locateInfo.displayTime = frameState.predictedDisplayTime;
            locateInfo.space = m_appSpace;

            uint32_t viewCount = 2;
            res = xrLocateViews(m_session, &locateInfo, &viewState, 2, &viewCount, m_views.data());

            bool trackingValid = (res == XR_SUCCESS && viewCount >= 2 &&
                (viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) != 0 &&
                (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) != 0);

            m_trackingValid = trackingValid;
            if (trackingValid)
            {
                XMFLOAT3 cockpitPos = mech.GetCockpitHeadPosition();
                float mechYaw = mech.GetYaw();
                XMVECTOR quatMechYaw = XMQuaternionRotationRollPitchYaw(0.0f, mechYaw, 0.0f);
                XMMATRIX rotMechY = XMMatrixRotationY(mechYaw);

                XrPosef centerPose = m_views[0].pose;
                centerPose.position.x = (m_views[0].pose.position.x + m_views[1].pose.position.x) * 0.5f;
                centerPose.position.y = (m_views[0].pose.position.y + m_views[1].pose.position.y) * 0.5f;
                centerPose.position.z = (m_views[0].pose.position.z + m_views[1].pose.position.z) * 0.5f;

                XMVECTOR hmdCenterLocal = XMVectorSet(centerPose.position.x, centerPose.position.y, -centerPose.position.z, 0.0f);
                XMVECTOR hmdCenterRot = XMVectorSet(-centerPose.orientation.x, -centerPose.orientation.y, centerPose.orientation.z, centerPose.orientation.w);

                XMVECTOR headWorldPos = XMVector3TransformCoord(hmdCenterLocal, rotMechY) + XMLoadFloat3(&cockpitPos);
                XMVECTOR headWorldRot = XMQuaternionMultiply(hmdCenterRot, quatMechYaw);

                XMStoreFloat3(&m_hmdWorldPos, headWorldPos);
                XMVECTOR fwd = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), headWorldRot);
                XMStoreFloat3(&m_hmdWorldForward, XMVector3Normalize(fwd));

                XMMATRIX headMat = XMMatrixRotationQuaternion(headWorldRot) * XMMatrixTranslationFromVector(headWorldPos);
                m_hmdView = XMMatrixInverse(nullptr, headMat);
                m_hmdProj = CreateProjectionFromFov(m_views[0].fov, 0.05f, 1000.0f);

                for (uint32_t i = 0; i < 2; ++i)
                {
                    auto& eye = m_eyes[i];

                    // 1. Acquire Image
                    XrSwapchainImageAcquireInfo acqInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
                    uint32_t imageIndex = 0;
                    res = xrAcquireSwapchainImage(eye.swapchain, &acqInfo, &imageIndex);
                    if (res != XR_SUCCESS) continue;

                    // 2. Wait for Image
                    XrSwapchainImageWaitInfo waitImageInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
                    waitImageInfo.timeout = XR_INFINITE_DURATION;
                    res = xrWaitSwapchainImage(eye.swapchain, &waitImageInfo);
                    if (res != XR_SUCCESS) continue;

                    // 3. Compute Eye Pose & Matrices
                    XrPosef eyePose = m_views[i].pose;
                    XMVECTOR hmdLocalPos = XMVectorSet(eyePose.position.x, eyePose.position.y, -eyePose.position.z, 0.0f);
                    XMVECTOR hmdLocalRot = XMVectorSet(-eyePose.orientation.x, -eyePose.orientation.y, eyePose.orientation.z, eyePose.orientation.w);

                    XMVECTOR eyeWorldPos = XMVector3TransformCoord(hmdLocalPos, rotMechY) + XMLoadFloat3(&cockpitPos);
                    XMVECTOR eyeWorldRot = XMQuaternionMultiply(hmdLocalRot, quatMechYaw);

                    XMMATRIX worldMatrix = XMMatrixRotationQuaternion(eyeWorldRot) * XMMatrixTranslationFromVector(eyeWorldPos);
                    m_eyeView[i] = XMMatrixInverse(nullptr, worldMatrix);
                    m_eyeProj[i] = CreateProjectionFromFov(m_views[i].fov, 0.05f, 1000.0f);

                    // 4. Render to Eye RTV
                    renderer->RenderVREye(
                        m_eyeView[i],
                        m_eyeProj[i],
                        eye.rtvs[imageIndex].Get(),
                        eye.dsv.Get(),
                        eye.viewport,
                        mech,
                        weapons,
                        targets,
                        lockSystem,
                        (i == 0),
                        remoteMechs,
                        network,
                        frames,
                        aiBots
                    );

                    // 5. Release Image
                    XrSwapchainImageReleaseInfo relInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
                    xrReleaseSwapchainImage(eye.swapchain, &relInfo);

                    // Setup projection view for layer
                    projViews[i].pose = m_views[i].pose;
                    projViews[i].fov  = m_views[i].fov;
                    projViews[i].subImage.swapchain = eye.swapchain;
                    projViews[i].subImage.imageRect.offset = { 0, 0 };
                    projViews[i].subImage.imageRect.extent = {
                        static_cast<int32_t>(eye.width),
                        static_cast<int32_t>(eye.height)
                    };
                    projViews[i].subImage.imageArrayIndex = 0;
                }

                projLayer.space = m_appSpace;
                projLayer.viewCount = 2;
                projLayer.views = projViews;
                rendered = true;
            }
        }

        // ALWAYS call xrEndFrame to guarantee compositor synchronization
        const XrCompositionLayerBaseHeader* layers[] = {
            reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projLayer)
        };

        XrFrameEndInfo endInfo = { XR_TYPE_FRAME_END_INFO };
        endInfo.displayTime = frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = rendered ? 1 : 0;
        endInfo.layers = rendered ? layers : nullptr;

        xrEndFrame(m_session, &endInfo);
        return rendered;
    }

    bool VRManager::RenderMenuFrame(
        D3D11Renderer* renderer,
        const MenuSystem& menu,
        const Camera& camera,
        const MechController& mech
    )
    {
        if (!m_isAvailable || !m_isSessionRunning || !renderer) return false;

        XrFrameWaitInfo waitInfo = { XR_TYPE_FRAME_WAIT_INFO };
        XrFrameState frameState = { XR_TYPE_FRAME_STATE };
        if (xrWaitFrame(m_session, &waitInfo, &frameState) != XR_SUCCESS) return false;

        XrFrameBeginInfo beginInfo = { XR_TYPE_FRAME_BEGIN_INFO };
        if (xrBeginFrame(m_session, &beginInfo) != XR_SUCCESS) return false;

        bool rendered = false;
        XrCompositionLayerProjection projLayer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
        XrCompositionLayerProjectionView projViews[2] = {
            { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW },
            { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW }
        };

        if (frameState.shouldRender == XR_TRUE)
        {
            XrViewState viewState = { XR_TYPE_VIEW_STATE };
            uint32_t viewCountOutput = 2;
            XrViewLocateInfo locateInfo = { XR_TYPE_VIEW_LOCATE_INFO };
            locateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            locateInfo.displayTime = frameState.predictedDisplayTime;
            locateInfo.space = m_appSpace;

            XrResult res = xrLocateViews(m_session, &locateInfo, &viewState, 2, &viewCountOutput, m_views.data());
            if (res == XR_SUCCESS && (viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT))
            {
                for (int i = 0; i < 2; ++i)
                {
                    VREyeData& eye = m_eyes[i];

                    XrSwapchainImageAcquireInfo acqInfo = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
                    uint32_t imageIndex = 0;
                    if (xrAcquireSwapchainImage(eye.swapchain, &acqInfo, &imageIndex) != XR_SUCCESS) continue;

                    XrSwapchainImageWaitInfo waitImageInfo = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
                    waitImageInfo.timeout = XR_INFINITE_DURATION;
                    if (xrWaitSwapchainImage(eye.swapchain, &waitImageInfo) != XR_SUCCESS) continue;

                    XrPosef eyePose = m_views[i].pose;
                    XMVECTOR hmdLocalPos = XMVectorSet(eyePose.position.x, eyePose.position.y, -eyePose.position.z, 0.0f);
                    XMVECTOR hmdLocalRot = XMVectorSet(-eyePose.orientation.x, -eyePose.orientation.y, eyePose.orientation.z, eyePose.orientation.w);

                    XMFLOAT3 camPos = camera.GetEyePosition();
                    XMVECTOR eyeWorldPos = hmdLocalPos + XMLoadFloat3(&camPos);
                    XMMATRIX worldMatrix = XMMatrixRotationQuaternion(hmdLocalRot) * XMMatrixTranslationFromVector(eyeWorldPos);
                    m_eyeView[i] = XMMatrixInverse(nullptr, worldMatrix);
                    m_eyeProj[i] = CreateProjectionFromFov(m_views[i].fov, 0.05f, 1000.0f);

                    ID3D11RenderTargetView* rtv = eye.rtvs[imageIndex].Get();
                    ID3D11DepthStencilView* dsv = eye.dsv.Get();
                    auto* context = renderer->GetContext();

                    const float clearColor[4] = { 0.015f, 0.02f, 0.035f, 1.0f };
                    context->ClearRenderTargetView(rtv, clearColor);
                    context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
                    context->OMSetRenderTargets(1, &rtv, dsv);
                    context->RSSetViewports(1, &eye.viewport);

                    renderer->RenderVRMainMenu(menu, m_eyeView[i], m_eyeProj[i]);

                    XrSwapchainImageReleaseInfo relInfo = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
                    xrReleaseSwapchainImage(eye.swapchain, &relInfo);

                    projViews[i].pose = m_views[i].pose;
                    projViews[i].fov  = m_views[i].fov;
                    projViews[i].subImage.swapchain = eye.swapchain;
                    projViews[i].subImage.imageRect.offset = { 0, 0 };
                    projViews[i].subImage.imageRect.extent = { static_cast<int32_t>(eye.width), static_cast<int32_t>(eye.height) };
                    projViews[i].subImage.imageArrayIndex = 0;
                }

                projLayer.space = m_appSpace;
                projLayer.viewCount = 2;
                projLayer.views = projViews;
                rendered = true;
            }
        }

        const XrCompositionLayerBaseHeader* layers[] = {
            reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projLayer)
        };

        XrFrameEndInfo endInfo = { XR_TYPE_FRAME_END_INFO };
        endInfo.displayTime = frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endInfo.layerCount = rendered ? 1 : 0;
        endInfo.layers = rendered ? layers : nullptr;

        xrEndFrame(m_session, &endInfo);
        return rendered;
    }
}
