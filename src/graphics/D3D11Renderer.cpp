#include "D3D11Renderer.hpp"
#include <d3dcompiler.h>
#include <iostream>
#include <cmath>

namespace Overdrive
{
    D3D11Renderer::D3D11Renderer()
    {
    }

    D3D11Renderer::~D3D11Renderer()
    {
        CleanupRenderTarget();
    }

    bool D3D11Renderer::Initialize(HWND hwnd, int width, int height)
    {
        m_hwnd = hwnd;
        m_width = width;
        m_height = height;

        if (!CreateDeviceAndSwapChain(hwnd)) return false;
        if (!CreateRenderTargetAndDepthBuffer()) return false;
        if (!InitShadersAndInputLayout()) return false;
        if (!m_gridFloor.Initialize(m_device.Get())) return false;
        if (!InitMechGeometry()) return false;
        if (!InitHUDGeometry()) return false;

        return true;
    }

    bool D3D11Renderer::CreateDeviceAndSwapChain(HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = 2;
        scd.BufferDesc.Width = m_width;
        scd.BufferDesc.Height = m_height;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferDesc.RefreshRate.Numerator = 60;
        scd.BufferDesc.RefreshRate.Denominator = 1;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = hwnd;
        scd.SampleDesc.Count = 1;
        scd.SampleDesc.Quality = 0;
        scd.Windowed = TRUE;
        scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        UINT createDeviceFlags = 0;
#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };
        D3D_FEATURE_LEVEL featureLevel;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createDeviceFlags,
            featureLevels,
            _countof(featureLevels),
            D3D11_SDK_VERSION,
            &scd,
            m_swapChain.GetAddressOf(),
            m_device.GetAddressOf(),
            &featureLevel,
            m_context.GetAddressOf()
        );

        if (FAILED(hr))
        {
            hr = D3D11CreateDeviceAndSwapChain(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                createDeviceFlags,
                &featureLevels[1],
                1,
                D3D11_SDK_VERSION,
                &scd,
                m_swapChain.GetAddressOf(),
                m_device.GetAddressOf(),
                &featureLevel,
                m_context.GetAddressOf()
            );
        }

        return SUCCEEDED(hr);
    }

    void D3D11Renderer::CleanupRenderTarget()
    {
        m_renderTargetView.Reset();
        m_depthStencilView.Reset();
        m_depthStencilBuffer.Reset();
    }

    bool D3D11Renderer::CreateRenderTargetAndDepthBuffer()
    {
        ComPtr<ID3D11Texture2D> backBuffer;
        HRESULT hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
        if (FAILED(hr)) return false;

        hr = m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.GetAddressOf());
        if (FAILED(hr)) return false;

        D3D11_TEXTURE2D_DESC depthDesc = {};
        depthDesc.Width = m_width;
        depthDesc.Height = m_height;
        depthDesc.MipLevels = 1;
        depthDesc.ArraySize = 1;
        depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthDesc.SampleDesc.Count = 1;
        depthDesc.SampleDesc.Quality = 0;
        depthDesc.Usage = D3D11_USAGE_DEFAULT;
        depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        hr = m_device->CreateTexture2D(&depthDesc, nullptr, m_depthStencilBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, m_depthStencilView.GetAddressOf());
        if (FAILED(hr)) return false;

        // Depth Stencil State (Standard 3D)
        D3D11_DEPTH_STENCIL_DESC dsDesc = {};
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

        hr = m_device->CreateDepthStencilState(&dsDesc, m_depthStencilState.GetAddressOf());
        if (FAILED(hr)) return false;

        // Depth Stencil State (HUD / Overlay - Depth test disabled)
        D3D11_DEPTH_STENCIL_DESC hudDsDesc = dsDesc;
        hudDsDesc.DepthEnable = FALSE;
        hudDsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

        hr = m_device->CreateDepthStencilState(&hudDsDesc, m_hudDepthDisabledState.GetAddressOf());
        if (FAILED(hr)) return false;

        // Rasterizer State
        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE; // Double sided for easy mesh visibility
        rasterDesc.DepthClipEnable = TRUE;

        hr = m_device->CreateRasterizerState(&rasterDesc, m_rasterizerState.GetAddressOf());
        return SUCCEEDED(hr);
    }

    void D3D11Renderer::Resize(int width, int height)
    {
        if (width <= 0 || height <= 0 || !m_swapChain) return;

        m_width = width;
        m_height = height;

        m_context->OMSetRenderTargets(0, nullptr, nullptr);
        CleanupRenderTarget();

        HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        if (SUCCEEDED(hr))
        {
            CreateRenderTargetAndDepthBuffer();
        }
    }

    bool D3D11Renderer::InitShadersAndInputLayout()
    {
        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#ifdef _DEBUG
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        const wchar_t* shaderPath = L"assets/shaders/basic.hlsl";

        ComPtr<ID3DBlob> vsBlob;
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(shaderPath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_0", compileFlags, 0, vsBlob.GetAddressOf(), errorBlob.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
        if (FAILED(hr)) return false;

        ComPtr<ID3DBlob> psBlob;
        hr = D3DCompileFromFile(shaderPath, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_0", compileFlags, 0, psBlob.GetAddressOf(), errorBlob.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());
        if (FAILED(hr)) return false;

        D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, color),    D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        hr = m_device->CreateInputLayout(layoutDesc, _countof(layoutDesc), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), m_inputLayout.GetAddressOf());
        if (FAILED(hr)) return false;

        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.ByteWidth = sizeof(TransformConstantBuffer);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = m_device->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());
        return SUCCEEDED(hr);
    }

    bool D3D11Renderer::InitMechGeometry()
    {
        // Construct detailed stylized Mech chassis matching user's pink/magenta & white AC6 unit
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        XMFLOAT4 mainPink    = { 0.85f, 0.28f, 0.48f, 1.0f }; // Magenta / rose pink armor
        XMFLOAT4 whitePlate  = { 0.92f, 0.94f, 0.96f, 1.0f }; // White armor panels
        XMFLOAT4 darkFrame   = { 0.18f, 0.20f, 0.24f, 1.0f }; // Internal carbon frame
        XMFLOAT4 thrusterCyan= { 0.25f, 0.80f, 1.00f, 1.0f }; // Glowing blue vernier nozzle

        auto addBox = [&](float cx, float cy, float cz, float sx, float sy, float sz, XMFLOAT4 col) {
            uint32_t base = static_cast<uint32_t>(vertices.size());
            float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;

            vertices.push_back({ { cx - hx, cy - hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });
            vertices.push_back({ { cx - hx, cy + hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });
            vertices.push_back({ { cx + hx, cy + hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });
            vertices.push_back({ { cx + hx, cy - hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });

            vertices.push_back({ { cx + hx, cy - hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });
            vertices.push_back({ { cx + hx, cy + hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });
            vertices.push_back({ { cx - hx, cy + hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });
            vertices.push_back({ { cx - hx, cy - hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });

            uint32_t bIdx[] = {
                0, 1, 2,  0, 2, 3,
                4, 5, 6,  4, 6, 7,
                1, 6, 5,  1, 5, 2,
                7, 0, 3,  7, 3, 4,
                7, 6, 1,  7, 1, 0,
                3, 2, 5,  3, 5, 4
            };

            for (uint32_t i : bIdx) indices.push_back(base + i);
        };

        // 1. Torso & Core
        addBox(0.0f, 1.6f, 0.0f, 0.9f, 0.8f, 0.7f, darkFrame);
        addBox(0.0f, 1.65f, 0.2f, 0.75f, 0.65f, 0.5f, whitePlate); // Chest plate
        addBox(0.0f, 1.85f, 0.15f, 0.45f, 0.35f, 0.4f, mainPink);  // Head / Visor

        // 2. Left & Right Shoulder Boosters (Wing Binders like AC6)
        addBox(-0.75f, 1.85f, -0.1f, 0.35f, 0.9f, 0.6f, mainPink);
        addBox( 0.75f, 1.85f, -0.1f, 0.35f, 0.9f, 0.6f, mainPink);
        addBox(-0.80f, 2.05f, -0.2f, 0.25f, 0.6f, 0.4f, whitePlate); // Wing fins
        addBox( 0.80f, 2.05f, -0.2f, 0.25f, 0.6f, 0.4f, whitePlate);

        // 3. Arms & Weapons
        addBox(-0.85f, 1.3f, 0.2f, 0.3f, 0.7f, 0.35f, darkFrame);
        addBox( 0.85f, 1.3f, 0.2f, 0.3f, 0.7f, 0.35f, darkFrame);
        addBox(-0.95f, 1.0f, 0.7f, 0.2f, 0.25f, 1.2f, whitePlate); // Left Rifle
        addBox( 0.95f, 1.0f, 0.7f, 0.2f, 0.25f, 1.2f, whitePlate); // Right Rifle

        // 4. Reverse-joint / Stylish Legs
        addBox(-0.35f, 0.65f, 0.0f, 0.3f, 1.3f, 0.4f, mainPink);
        addBox( 0.35f, 0.65f, 0.0f, 0.3f, 1.3f, 0.4f, mainPink);
        addBox(-0.35f, 0.1f, 0.15f, 0.35f, 0.2f, 0.7f, darkFrame); // Feet
        addBox( 0.35f, 0.1f, 0.15f, 0.35f, 0.2f, 0.7f, darkFrame);

        // 5. Back Thrusters (Cyan glowing vernier ports)
        addBox(-0.35f, 1.6f, -0.45f, 0.22f, 0.22f, 0.2f, thrusterCyan);
        addBox( 0.35f, 1.6f, -0.45f, 0.22f, 0.22f, 0.2f, thrusterCyan);

        m_mechIndexCount = static_cast<UINT>(indices.size());

        D3D11_BUFFER_DESC vbd = {};
        vbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vdata = {};
        vdata.pSysMem = vertices.data();

        HRESULT hr = m_device->CreateBuffer(&vbd, &vdata, m_mechVertexBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        D3D11_BUFFER_DESC ibd = {};
        ibd.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA idata = {};
        idata.pSysMem = indices.data();

        hr = m_device->CreateBuffer(&ibd, &idata, m_mechIndexBuffer.GetAddressOf());
        return SUCCEEDED(hr);
    }

    bool D3D11Renderer::InitHUDGeometry()
    {
        // 1. Static Reticle Geometry (Line List in NDC coordinates [-1, 1])
        std::vector<Vertex> reticleVertices;
        XMFLOAT4 hudCyan = { 0.4f, 0.85f, 1.0f, 0.85f };

        // Center crosshair
        reticleVertices.push_back({ { -0.02f,  0.0f, 0.0f }, { 0, 0, 1 }, hudCyan });
        reticleVertices.push_back({ {  0.02f,  0.0f, 0.0f }, { 0, 0, 1 }, hudCyan });
        reticleVertices.push_back({ {  0.0f, -0.035f, 0.0f }, { 0, 0, 1 }, hudCyan });
        reticleVertices.push_back({ {  0.0f,  0.035f, 0.0f }, { 0, 0, 1 }, hudCyan });

        // Circular lock-on ring (32 segments)
        const int segments = 32;
        const float radiusX = 0.12f;
        const float radiusY = 0.21f; // Compensate for 16:9 aspect ratio

        for (int i = 0; i < segments; ++i)
        {
            float a1 = (XM_2PI * i) / segments;
            float a2 = (XM_2PI * (i + 1)) / segments;

            reticleVertices.push_back({ { std::cos(a1) * radiusX, std::sin(a1) * radiusY, 0.0f }, { 0, 0, 1 }, hudCyan });
            reticleVertices.push_back({ { std::cos(a2) * radiusX, std::sin(a2) * radiusY, 0.0f }, { 0, 0, 1 }, hudCyan });
        }

        // Horizontal artificial horizon dashes
        reticleVertices.push_back({ { -0.25f, 0.0f, 0.0f }, { 0, 0, 1 }, hudCyan });
        reticleVertices.push_back({ { -0.16f, 0.0f, 0.0f }, { 0, 0, 1 }, hudCyan });
        reticleVertices.push_back({ {  0.16f, 0.0f, 0.0f }, { 0, 0, 1 }, hudCyan });
        reticleVertices.push_back({ {  0.25f, 0.0f, 0.0f }, { 0, 0, 1 }, hudCyan });

        m_reticleVertexCount = static_cast<UINT>(reticleVertices.size());

        D3D11_BUFFER_DESC rbd = {};
        rbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * reticleVertices.size());
        rbd.Usage = D3D11_USAGE_IMMUTABLE;
        rbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA rdata = {};
        rdata.pSysMem = reticleVertices.data();

        HRESULT hr = m_device->CreateBuffer(&rbd, &rdata, m_hudVertexBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        // 2. Dynamic Buffer for Energy Gauge (Quad / Triangles)
        D3D11_BUFFER_DESC enDesc = {};
        enDesc.ByteWidth = sizeof(Vertex) * 12; // 6 for frame + 6 for fill bar
        enDesc.Usage = D3D11_USAGE_DYNAMIC;
        enDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        enDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = m_device->CreateBuffer(&enDesc, nullptr, m_dynamicEnBuffer.GetAddressOf());
        return SUCCEEDED(hr);
    }

    void D3D11Renderer::BeginFrame()
    {
        // Dark AC test ground background
        const float clearColor[4] = { 0.06f, 0.07f, 0.10f, 1.0f };
        m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
        m_context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        D3D11_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(m_width);
        viewport.Height = static_cast<float>(m_height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        m_context->RSSetViewports(1, &viewport);
        m_context->RSSetState(m_rasterizerState.Get());
        m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());
    }

    void D3D11Renderer::RenderScene(const Camera& camera, const MechController& mech)
    {
        float aspect = static_cast<float>(m_width) / static_cast<float>(m_height > 0 ? m_height : 1);
        XMMATRIX view = camera.GetViewMatrix();
        XMMATRIX proj = camera.GetProjectionMatrix(aspect);

        // 1. Render Grid Floor & Hangar Walls
        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = XMMatrixIdentity();
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // Emissive grid
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
        m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        m_context->IASetInputLayout(m_inputLayout.Get());

        m_gridFloor.Render(m_context.Get());

        // 2. Render Mech
        bool isFPV = (camera.GetMode() == CameraMode::FPV);
        RenderMech(mech, view, proj, isFPV);

        // 3. Render HUD (Reticle & EN Bar)
        RenderHUD(mech, camera.GetMode());
    }

    void D3D11Renderer::RenderMech(const MechController& mech, const XMMATRIX& view, const XMMATRIX& proj, bool isFPV)
    {
        // Calculate Mech World Matrix (Yaw, Pitch, Roll and Position)
        XMFLOAT3 pos = mech.GetPosition();

        // In Assault Boost, tilt forward slightly
        float abPitch = mech.IsAssaultBoost() ? 0.35f : 0.0f;
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(mech.GetPitch() * 0.3f + abPitch, mech.GetYaw(), mech.GetRoll());
        XMMATRIX trans = XMMatrixTranslation(pos.x, pos.y, pos.z);
        XMMATRIX world = rot * trans;

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = world;
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f); // Lit Mech
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_mechVertexBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetIndexBuffer(m_mechIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        if (isFPV)
        {
            // Skip torso, chest plate, and head/visor (first 108 indices)
            // This prevents the mech body from clipping into the camera when looking down in FPV
            if (m_mechIndexCount > 108)
            {
                m_context->DrawIndexed(m_mechIndexCount - 108, 108, 0);
            }
        }
        else
        {
            // TPS: Draw entire mech
            m_context->DrawIndexed(m_mechIndexCount, 0, 0);
        }
    }

    void D3D11Renderer::RenderHUD(const MechController& mech, CameraMode cameraMode)
    {
        // Set Orthographic / Identity projection for 2D HUD rendering
        m_context->OMSetDepthStencilState(m_hudDepthDisabledState.Get(), 0);

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = XMMatrixIdentity();
            cb->view = XMMatrixIdentity();
            cb->projection = XMMatrixIdentity();
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // Emissive unlit
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        UINT stride = sizeof(Vertex);
        UINT offset = 0;

        // 1. Draw Reticle
        m_context->IASetVertexBuffers(0, 1, m_hudVertexBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        m_context->Draw(m_reticleVertexCount, 0);

        // 2. Update and Draw Energy (EN) Bar at the bottom center
        float enRatio = mech.GetEnergyRatio();
        float left = -0.35f;
        float right = 0.35f;
        float top = -0.76f;
        float bottom = -0.79f;

        float fillRight = left + (right - left) * enRatio;

        XMFLOAT4 frameColor = { 0.25f, 0.30f, 0.38f, 0.7f };
        XMFLOAT4 enFillColor = (enRatio > 0.25f) ? XMFLOAT4(0.20f, 0.75f, 1.00f, 0.95f) // Bright Cyan EN
                                                 : XMFLOAT4(1.00f, 0.25f, 0.20f, 0.95f); // Red Alert when low

        std::vector<Vertex> enVerts = {
            // Frame / Background Bar
            { { left,  bottom, 0.0f }, { 0, 0, 1 }, frameColor },
            { { left,  top,    0.0f }, { 0, 0, 1 }, frameColor },
            { { right, top,    0.0f }, { 0, 0, 1 }, frameColor },
            { { left,  bottom, 0.0f }, { 0, 0, 1 }, frameColor },
            { { right, top,    0.0f }, { 0, 0, 1 }, frameColor },
            { { right, bottom, 0.0f }, { 0, 0, 1 }, frameColor },

            // Active EN Fill Bar
            { { left,      bottom, 0.0f }, { 0, 0, 1 }, enFillColor },
            { { left,      top,    0.0f }, { 0, 0, 1 }, enFillColor },
            { { fillRight, top,    0.0f }, { 0, 0, 1 }, enFillColor },
            { { left,      bottom, 0.0f }, { 0, 0, 1 }, enFillColor },
            { { fillRight, top,    0.0f }, { 0, 0, 1 }, enFillColor },
            { { fillRight, bottom, 0.0f }, { 0, 0, 1 }, enFillColor },
        };

        if (SUCCEEDED(m_context->Map(m_dynamicEnBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, enVerts.data(), sizeof(Vertex) * enVerts.size());
            m_context->Unmap(m_dynamicEnBuffer.Get(), 0);
        }

        m_context->IASetVertexBuffers(0, 1, m_dynamicEnBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->Draw(12, 0);
    }

    void D3D11Renderer::EndFrame()
    {
        m_swapChain->Present(1, 0);
    }
}
