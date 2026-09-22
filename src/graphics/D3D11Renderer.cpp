#include "D3D11Renderer.hpp"
#include "combat/TargetLockSystem.hpp"
#include <d3dcompiler.h>
#include <iostream>
#include <cmath>
#include <string>

namespace Overdrive
{
    namespace
    {
        // 7-segment / stroke vector font for HUD distance and target assist display
        void AddCharLines(char c, float cx, float cy, float w, float h, XMFLOAT4 col, std::vector<Vertex>& out)
        {
            float l = cx - w * 0.5f;
            float r = cx + w * 0.5f;
            float t = cy + h * 0.5f;
            float m = cy;
            float b = cy - h * 0.5f;

            auto addL = [&](float x1, float y1, float x2, float y2) {
                out.push_back({ { x1, y1, 0.0f }, { 0, 0, 1 }, col });
                out.push_back({ { x2, y2, 0.0f }, { 0, 0, 1 }, col });
            };

            switch (c)
            {
            case '0':
                addL(l, t, r, t); addL(r, t, r, b); addL(r, b, l, b); addL(l, b, l, t);
                break;
            case '1':
                addL(r, t, r, b);
                break;
            case '2':
                addL(l, t, r, t); addL(r, t, r, m); addL(r, m, l, m); addL(l, m, l, b); addL(l, b, r, b);
                break;
            case '3':
                addL(l, t, r, t); addL(r, t, r, b); addL(l, b, r, b); addL(l, m, r, m);
                break;
            case '4':
                addL(l, t, l, m); addL(l, m, r, m); addL(r, t, r, b);
                break;
            case '5':
                addL(r, t, l, t); addL(l, t, l, m); addL(l, m, r, m); addL(r, m, r, b); addL(r, b, l, b);
                break;
            case '6':
                addL(r, t, l, t); addL(l, t, l, b); addL(l, b, r, b); addL(r, b, r, m); addL(r, m, l, m);
                break;
            case '7':
                addL(l, t, r, t); addL(r, t, r, b);
                break;
            case '8':
                addL(l, t, r, t); addL(r, t, r, b); addL(r, b, l, b); addL(l, b, l, t); addL(l, m, r, m);
                break;
            case '9':
                addL(r, m, l, m); addL(l, m, l, t); addL(l, t, r, t); addL(r, t, r, b); addL(r, b, l, b);
                break;
            case 'M':
                addL(l, b, l, t); addL(l, t, cx, m); addL(cx, m, r, t); addL(r, t, r, b);
                break;
            case '[':
                addL(r, t, l, t); addL(l, t, l, b); addL(l, b, r, b);
                break;
            case ']':
                addL(l, t, r, t); addL(r, t, r, b); addL(r, b, l, b);
                break;
            case '-':
                addL(l, m, r, m);
                break;
            case 'T':
                addL(l, t, r, t); addL(cx, t, cx, b);
                break;
            case 'A':
                addL(l, b, l, t); addL(l, t, r, t); addL(r, t, r, b); addL(l, m, r, m);
                break;
            case 'R':
                addL(l, b, l, t); addL(l, t, r, t); addL(r, t, r, m); addL(r, m, l, m); addL(cx, m, r, b);
                break;
            case 'G':
                addL(r, t, l, t); addL(l, t, l, b); addL(l, b, r, b); addL(r, b, r, m); addL(r, m, cx, m);
                break;
            case 'E':
                addL(r, t, l, t); addL(l, t, l, b); addL(l, b, r, b); addL(l, m, r, m);
                break;
            case 'S':
                addL(r, t, l, t); addL(l, t, l, m); addL(l, m, r, m); addL(r, m, r, b); addL(r, b, l, b);
                break;
            case 'I':
                addL(cx, t, cx, b); addL(l, t, r, t); addL(l, b, r, b);
                break;
            default:
                break;
            }
        }

        void AddStringLines(const std::string& str, float startX, float startY, float charW, float charH, float spacing, XMFLOAT4 col, std::vector<Vertex>& out)
        {
            float x = startX;
            for (char c : str)
            {
                if (c != ' ')
                {
                    AddCharLines(c, x, startY, charW, charH, col, out);
                }
                x += charW + spacing;
            }
        }
    }
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
        if (!InitCombatGeometry()) return false;

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
        rasterDesc.CullMode = D3D11_CULL_NONE;
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
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        XMFLOAT4 mainPink    = { 0.85f, 0.28f, 0.48f, 1.0f };
        XMFLOAT4 whitePlate  = { 0.92f, 0.94f, 0.96f, 1.0f };
        XMFLOAT4 darkFrame   = { 0.18f, 0.20f, 0.24f, 1.0f };
        XMFLOAT4 thrusterCyan= { 0.25f, 0.80f, 1.00f, 1.0f };

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

        // 1. Torso & Core (3 boxes = 108 indices, skipped in FPV)
        addBox(0.0f, 1.6f, 0.0f, 0.9f, 0.8f, 0.7f, darkFrame);
        addBox(0.0f, 1.65f, 0.2f, 0.75f, 0.65f, 0.5f, whitePlate);
        addBox(0.0f, 1.85f, 0.15f, 0.45f, 0.35f, 0.4f, mainPink);

        // 2. Left & Right Shoulder Boosters
        addBox(-0.75f, 1.85f, -0.1f, 0.35f, 0.9f, 0.6f, mainPink);
        addBox( 0.75f, 1.85f, -0.1f, 0.35f, 0.9f, 0.6f, mainPink);
        addBox(-0.80f, 2.05f, -0.2f, 0.25f, 0.6f, 0.4f, whitePlate);
        addBox( 0.80f, 2.05f, -0.2f, 0.25f, 0.6f, 0.4f, whitePlate);

        // 3. Arms & Weapons
        addBox(-0.85f, 1.3f, 0.2f, 0.3f, 0.7f, 0.35f, darkFrame);
        addBox( 0.85f, 1.3f, 0.2f, 0.3f, 0.7f, 0.35f, darkFrame);
        addBox(-0.95f, 1.0f, 0.7f, 0.2f, 0.25f, 1.2f, whitePlate); // Left Rifle
        addBox( 0.95f, 1.0f, 0.7f, 0.2f, 0.25f, 1.2f, whitePlate); // Right Rifle

        // 4. Reverse-joint Legs
        addBox(-0.35f, 0.65f, 0.0f, 0.3f, 1.3f, 0.4f, mainPink);
        addBox( 0.35f, 0.65f, 0.0f, 0.3f, 1.3f, 0.4f, mainPink);
        addBox(-0.35f, 0.1f, 0.15f, 0.35f, 0.2f, 0.7f, darkFrame);
        addBox( 0.35f, 0.1f, 0.15f, 0.35f, 0.2f, 0.7f, darkFrame);

        // 5. Back Thrusters
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
        std::vector<Vertex> reticleVertices;
        XMFLOAT4 hudCyan = { 0.35f, 0.85f, 1.0f, 0.80f };
        XMFLOAT4 hudDim  = { 0.25f, 0.55f, 0.70f, 0.50f };

        // 1. Center neutral dot / small crosshair
        reticleVertices.push_back({ { -0.008f,  0.0f, 0.0f }, { 0, 0, 1 }, hudCyan });
        reticleVertices.push_back({ {  0.008f,  0.0f, 0.0f }, { 0, 0, 1 }, hudCyan });
        reticleVertices.push_back({ {  0.0f, -0.014f, 0.0f }, { 0, 0, 1 }, hudCyan });
        reticleVertices.push_back({ {  0.0f,  0.014f, 0.0f }, { 0, 0, 1 }, hudCyan });

        // 2. AC6-style outer reticle arc brackets (Left and Right curved arches)
        const float radX = 0.165f;
        const float radY = 0.285f;
        const int arcSegs = 14;

        // Left Arc: 120 deg to 240 deg (in radians: ~2.09 to 4.19)
        float startAngleL = XMConvertToRadians(125.0f);
        float endAngleL   = XMConvertToRadians(235.0f);
        for (int i = 0; i < arcSegs; ++i)
        {
            float a1 = startAngleL + (endAngleL - startAngleL) * (float(i) / arcSegs);
            float a2 = startAngleL + (endAngleL - startAngleL) * (float(i + 1) / arcSegs);
            reticleVertices.push_back({ { std::cos(a1) * radX, std::sin(a1) * radY, 0.0f }, { 0, 0, 1 }, hudCyan });
            reticleVertices.push_back({ { std::cos(a2) * radX, std::sin(a2) * radY, 0.0f }, { 0, 0, 1 }, hudCyan });
        }

        // Right Arc: -55 deg to +55 deg (in radians: ~ -0.96 to 0.96)
        float startAngleR = XMConvertToRadians(-55.0f);
        float endAngleR   = XMConvertToRadians(55.0f);
        for (int i = 0; i < arcSegs; ++i)
        {
            float a1 = startAngleR + (endAngleR - startAngleR) * (float(i) / arcSegs);
            float a2 = startAngleR + (endAngleR - startAngleR) * (float(i + 1) / arcSegs);
            reticleVertices.push_back({ { std::cos(a1) * radX, std::sin(a1) * radY, 0.0f }, { 0, 0, 1 }, hudCyan });
            reticleVertices.push_back({ { std::cos(a2) * radX, std::sin(a2) * radY, 0.0f }, { 0, 0, 1 }, hudCyan });
        }

        // Top arc ticks (AC6 upper gauge marks)
        const float topRadX = 0.190f;
        const float topRadY = 0.325f;
        for (int i = 0; i <= 6; ++i)
        {
            float a = XMConvertToRadians(65.0f + i * 8.33f);
            float x1 = std::cos(a) * radX;
            float y1 = std::sin(a) * radY;
            float x2 = std::cos(a) * topRadX;
            float y2 = std::sin(a) * topRadY;
            reticleVertices.push_back({ { x1, y1, 0.0f }, { 0, 0, 1 }, hudDim });
            reticleVertices.push_back({ { x2, y2, 0.0f }, { 0, 0, 1 }, hudDim });
        }

        // 3. Horizon reference dashes
        reticleVertices.push_back({ { -0.30f, 0.0f, 0.0f }, { 0, 0, 1 }, hudDim });
        reticleVertices.push_back({ { -0.22f, 0.0f, 0.0f }, { 0, 0, 1 }, hudCyan });
        reticleVertices.push_back({ {  0.22f, 0.0f, 0.0f }, { 0, 0, 1 }, hudCyan });
        reticleVertices.push_back({ {  0.30f, 0.0f, 0.0f }, { 0, 0, 1 }, hudDim });

        m_reticleVertexCount = static_cast<UINT>(reticleVertices.size());

        D3D11_BUFFER_DESC rbd = {};
        rbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * reticleVertices.size());
        rbd.Usage = D3D11_USAGE_IMMUTABLE;
        rbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA rdata = {};
        rdata.pSysMem = reticleVertices.data();

        HRESULT hr = m_device->CreateBuffer(&rbd, &rdata, m_hudVertexBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        // Dynamic EN Gauge buffer
        D3D11_BUFFER_DESC enDesc = {};
        enDesc.ByteWidth = sizeof(Vertex) * 12;
        enDesc.Usage = D3D11_USAGE_DYNAMIC;
        enDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        enDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = m_device->CreateBuffer(&enDesc, nullptr, m_dynamicEnBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        // Dynamic Reticle Buffer (Inner red reticle, distance text, target assist label: 512 vertices)
        D3D11_BUFFER_DESC dynRetDesc = {};
        dynRetDesc.ByteWidth = sizeof(Vertex) * 512;
        dynRetDesc.Usage = D3D11_USAGE_DYNAMIC;
        dynRetDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        dynRetDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = m_device->CreateBuffer(&dynRetDesc, nullptr, m_dynamicReticleBuffer.GetAddressOf());
        return SUCCEEDED(hr);
    }

    bool D3D11Renderer::InitCombatGeometry()
    {
        // 1. Dynamic buffer for projectiles (up to 128 active projectiles = 256 vertices for LineList)
        D3D11_BUFFER_DESC pDesc = {};
        pDesc.ByteWidth = sizeof(Vertex) * 256;
        pDesc.Usage = D3D11_USAGE_DYNAMIC;
        pDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        pDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = m_device->CreateBuffer(&pDesc, nullptr, m_dynamicProjectileBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        // 2. Dynamic buffer for Ammo HUD bars (2 bars x 12 vertices = 24 vertices)
        D3D11_BUFFER_DESC ammoDesc = {};
        ammoDesc.ByteWidth = sizeof(Vertex) * 24;
        ammoDesc.Usage = D3D11_USAGE_DYNAMIC;
        ammoDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        ammoDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = m_device->CreateBuffer(&ammoDesc, nullptr, m_dynamicAmmoBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        // 3. Target Dummy Geometry (Standard test drone mesh: main pod + sensor dome)
        std::vector<Vertex> dVerts;
        std::vector<uint32_t> dIndices;

        XMFLOAT4 targetOrange = { 0.95f, 0.50f, 0.15f, 1.0f };
        XMFLOAT4 darkFrame    = { 0.20f, 0.22f, 0.26f, 1.0f };
        XMFLOAT4 eyeCyan      = { 0.30f, 0.85f, 1.00f, 1.0f };

        auto addBox = [&](float cx, float cy, float cz, float sx, float sy, float sz, XMFLOAT4 col) {
            uint32_t base = static_cast<uint32_t>(dVerts.size());
            float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;

            dVerts.push_back({ { cx - hx, cy - hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });
            dVerts.push_back({ { cx - hx, cy + hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });
            dVerts.push_back({ { cx + hx, cy + hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });
            dVerts.push_back({ { cx + hx, cy - hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });

            dVerts.push_back({ { cx + hx, cy - hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });
            dVerts.push_back({ { cx + hx, cy + hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });
            dVerts.push_back({ { cx - hx, cy + hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });
            dVerts.push_back({ { cx - hx, cy - hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });

            uint32_t bIdx[] = {
                0, 1, 2,  0, 2, 3,
                4, 5, 6,  4, 6, 7,
                1, 6, 5,  1, 5, 2,
                7, 0, 3,  7, 3, 4,
                7, 6, 1,  7, 1, 0,
                3, 2, 5,  3, 5, 4
            };

            for (uint32_t i : bIdx) dIndices.push_back(base + i);
        };

        // Main armor pod
        addBox(0.0f, 0.0f, 0.0f, 1.8f, 2.4f, 1.8f, targetOrange);
        // Base plate
        addBox(0.0f, -1.3f, 0.0f, 2.2f, 0.3f, 2.2f, darkFrame);
        // Sensor visor
        addBox(0.0f, 0.4f, 0.95f, 1.0f, 0.4f, 0.2f, eyeCyan);

        m_dummyIndexCount = static_cast<UINT>(dIndices.size());

        D3D11_BUFFER_DESC vbd = {};
        vbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * dVerts.size());
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vdata = {};
        vdata.pSysMem = dVerts.data();

        hr = m_device->CreateBuffer(&vbd, &vdata, m_dummyVertexBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        D3D11_BUFFER_DESC ibd = {};
        ibd.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * dIndices.size());
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA idata = {};
        idata.pSysMem = dIndices.data();

        hr = m_device->CreateBuffer(&ibd, &idata, m_dummyIndexBuffer.GetAddressOf());
        return SUCCEEDED(hr);
    }

    void D3D11Renderer::BeginFrame()
    {
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

    void D3D11Renderer::RenderScene(
        const Camera& camera,
        const MechController& mech,
        const WeaponSystem& weapons,
        const std::vector<TargetDummy>& targets,
        const TargetLockSystem& lockSystem)
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

        // 2. Render Target Dummies
        RenderTargetDummies(targets, view, proj);

        // 3. Render Mech
        bool isFPV = (camera.GetMode() == CameraMode::FPV);
        RenderMech(mech, view, proj, isFPV);

        // 4. Render Projectiles (High-energy glowing laser/bullet tracers)
        RenderProjectiles(weapons, view, proj);

        // 5. Render HUD (Reticle, Dynamic Inner Reticle, Distance Readout, EN Bar, Ammo Bars)
        RenderHUD(mech, weapons, lockSystem, camera.GetMode());
    }

    void D3D11Renderer::RenderTargetDummies(const std::vector<TargetDummy>& targets, const XMMATRIX& view, const XMMATRIX& proj)
    {
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_dummyVertexBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetIndexBuffer(m_dummyIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (const auto& target : targets)
        {
            if (target.IsDestroyed()) continue;

            XMFLOAT3 pos = target.GetPosition();
            XMMATRIX world = XMMatrixTranslation(pos.x, pos.y, pos.z);

            D3D11_MAPPED_SUBRESOURCE mapped;
            if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
                cb->world = world;
                cb->view = view;
                cb->projection = proj;
                // If flashing white from hit, use unlit mode (1.0 in customParams) or lit
                cb->customParams = target.IsHitFlashing() ? XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) : XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
                m_context->Unmap(m_constantBuffer.Get(), 0);
            }

            m_context->DrawIndexed(m_dummyIndexCount, 0, 0);
        }
    }

    void D3D11Renderer::RenderProjectiles(const WeaponSystem& weapons, const XMMATRIX& view, const XMMATRIX& proj)
    {
        const auto& projectiles = weapons.GetProjectiles();
        if (projectiles.empty()) return;

        std::vector<Vertex> pVerts;
        pVerts.reserve(projectiles.size() * 2);

        for (const auto& p : projectiles)
        {
            // Draw glowing tracer line from prevPosition to current position
            pVerts.push_back({ p.prevPosition, { 0, 1, 0 }, p.color });
            pVerts.push_back({ p.position,     { 0, 1, 0 }, p.color });
        }

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_dynamicProjectileBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, pVerts.data(), sizeof(Vertex) * pVerts.size());
            m_context->Unmap(m_dynamicProjectileBuffer.Get(), 0);
        }

        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = XMMatrixIdentity();
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // Emissive unlit beam
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_dynamicProjectileBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        m_context->Draw(static_cast<UINT>(pVerts.size()), 0);
    }

    void D3D11Renderer::RenderMech(const MechController& mech, const XMMATRIX& view, const XMMATRIX& proj, bool isFPV)
    {
        XMFLOAT3 pos = mech.GetPosition();

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
            if (m_mechIndexCount > 108)
            {
                m_context->DrawIndexed(m_mechIndexCount - 108, 108, 0);
            }
        }
        else
        {
            m_context->DrawIndexed(m_mechIndexCount, 0, 0);
        }
    }

    void D3D11Renderer::RenderHUD(const MechController& mech, const WeaponSystem& weapons, const TargetLockSystem& lockSystem, CameraMode cameraMode)
    {
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

        // 1. Draw Static Outer Reticle & Horizon
        m_context->IASetVertexBuffers(0, 1, m_hudVertexBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        m_context->Draw(m_reticleVertexCount, 0);

        // 2. Draw Dynamic Inner Reticle & Distance Meter (AC6 FCS Soft-Lock & Hard-Lock)
        const auto& targetInfo = lockSystem.GetCurrentTarget();
        XMFLOAT2 retNdc = lockSystem.GetInnerReticlePos();
        bool isHardLock = lockSystem.IsHardLockEnabled();

        std::vector<Vertex> dynRetVerts;
        dynRetVerts.reserve(384);

        XMFLOAT4 retRed    = { 1.0f, 0.22f, 0.18f, 0.95f }; // AC6 bright warning red
        XMFLOAT4 retYellow = { 1.0f, 0.88f, 0.25f, 0.95f }; // Target assist yellow
        XMFLOAT4 distWhite = { 0.92f, 0.92f, 0.96f, 0.90f };

        float aspect = static_cast<float>(m_width) / static_cast<float>(m_height > 0 ? m_height : 1);
        float invAspect = 1.0f / aspect;

        // If target is detected or hard lock is engaged, draw dynamic inner reticle (Image 1 & 2)
        if (targetInfo.hasTarget || isHardLock)
        {
            float rx = retNdc.x;
            float ry = retNdc.y;

            // Center small crosshair
            float cw = 0.010f * invAspect;
            float ch = 0.010f;
            dynRetVerts.push_back({ { rx - cw, ry, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx + cw, ry, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx, ry - ch, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx, ry + ch, 0.0f }, { 0, 0, 1 }, retRed });

            // 4-corner brackets: [ + ]
            float bw = 0.038f * invAspect;
            float bh = 0.038f;
            float blenX = 0.012f * invAspect;
            float blenY = 0.012f;

            // Top-Left corner
            dynRetVerts.push_back({ { rx - bw, ry + bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx - bw + blenX, ry + bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx - bw, ry + bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx - bw, ry + bh - blenY, 0.0f }, { 0, 0, 1 }, retRed });

            // Top-Right corner
            dynRetVerts.push_back({ { rx + bw, ry + bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx + bw - blenX, ry + bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx + bw, ry + bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx + bw, ry + bh - blenY, 0.0f }, { 0, 0, 1 }, retRed });

            // Bottom-Left corner
            dynRetVerts.push_back({ { rx - bw, ry - bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx - bw + blenX, ry - bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx - bw, ry - bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx - bw, ry - bh + blenY, 0.0f }, { 0, 0, 1 }, retRed });

            // Bottom-Right corner
            dynRetVerts.push_back({ { rx + bw, ry - bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx + bw - blenX, ry - bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx + bw, ry - bh, 0.0f }, { 0, 0, 1 }, retRed });
            dynRetVerts.push_back({ { rx + bw, ry - bh + blenY, 0.0f }, { 0, 0, 1 }, retRed });

            // Distance readout: e.g. "[ 125M ]" right below the reticle (as seen in Image 1 & 2)
            if (targetInfo.hasTarget)
            {
                int distVal = static_cast<int>(targetInfo.distance);
                std::string distStr = "[ " + std::to_string(distVal) + "M ]";
                float charH = 0.022f;
                float charW = charH * invAspect * 0.75f;
                float spacing = charW * 0.4f;
                float totalW = distStr.length() * (charW + spacing);
                float textX = rx - totalW * 0.5f;
                float textY = ry - bh - 0.035f;

                AddStringLines(distStr, textX, textY, charW, charH, spacing, distWhite, dynRetVerts);
            }
        }

        // Hard-Lock (Target Assist) top-center indicator
        if (isHardLock)
        {
            std::string assistStr = "[ TARGET ASSIST ]";
            float charH = 0.024f;
            float charW = charH * invAspect * 0.75f;
            float spacing = charW * 0.4f;
            float totalW = assistStr.length() * (charW + spacing);
            float textX = -totalW * 0.5f;
            float textY = 0.72f;

            AddStringLines(assistStr, textX, textY, charW, charH, spacing, retYellow, dynRetVerts);
        }

        if (!dynRetVerts.empty())
        {
            if (SUCCEEDED(m_context->Map(m_dynamicReticleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                memcpy(mapped.pData, dynRetVerts.data(), sizeof(Vertex) * dynRetVerts.size());
                m_context->Unmap(m_dynamicReticleBuffer.Get(), 0);
            }

            m_context->IASetVertexBuffers(0, 1, m_dynamicReticleBuffer.GetAddressOf(), &stride, &offset);
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            m_context->Draw(static_cast<UINT>(dynRetVerts.size()), 0);
        }

        // 3. Update and Draw Energy (EN) Bar
        float enRatio = mech.GetEnergyRatio();
        float left = -0.35f;
        float right = 0.35f;
        float top = -0.76f;
        float bottom = -0.79f;

        float fillRight = left + (right - left) * enRatio;

        XMFLOAT4 frameColor = { 0.25f, 0.30f, 0.38f, 0.7f };
        XMFLOAT4 enFillColor = (enRatio > 0.25f) ? XMFLOAT4(0.20f, 0.75f, 1.00f, 0.95f)
                                                 : XMFLOAT4(1.00f, 0.25f, 0.20f, 0.95f);

        std::vector<Vertex> enVerts = {
            // Frame
            { { left,  bottom, 0.0f }, { 0, 0, 1 }, frameColor },
            { { left,  top,    0.0f }, { 0, 0, 1 }, frameColor },
            { { right, top,    0.0f }, { 0, 0, 1 }, frameColor },
            { { left,  bottom, 0.0f }, { 0, 0, 1 }, frameColor },
            { { right, top,    0.0f }, { 0, 0, 1 }, frameColor },
            { { right, bottom, 0.0f }, { 0, 0, 1 }, frameColor },

            // Fill Bar
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

        // 4. Update and Draw Weapon Ammo Bars (Right Arm & Left Arm in bottom-right)
        float rAmmoRatio = static_cast<float>(weapons.GetRightAmmo()) / static_cast<float>(weapons.GetMaxAmmo());
        float lAmmoRatio = static_cast<float>(weapons.GetLeftAmmo()) / static_cast<float>(weapons.GetMaxAmmo());

        XMFLOAT4 raColor = { 1.0f, 0.60f, 0.20f, 0.9f }; // Orange
        XMFLOAT4 laColor = { 0.3f, 0.85f, 1.00f, 0.9f }; // Cyan

        float aLeft = 0.55f, aRight = 0.85f;
        float raTop = -0.74f, raBot = -0.765f;
        float laTop = -0.79f, laBot = -0.815f;

        float rFill = aLeft + (aRight - aLeft) * rAmmoRatio;
        float lFill = aLeft + (aRight - aLeft) * lAmmoRatio;

        std::vector<Vertex> ammoVerts = {
            // RA Fill
            { { aLeft, raBot, 0.0f }, { 0, 0, 1 }, raColor },
            { { aLeft, raTop, 0.0f }, { 0, 0, 1 }, raColor },
            { { rFill, raTop, 0.0f }, { 0, 0, 1 }, raColor },
            { { aLeft, raBot, 0.0f }, { 0, 0, 1 }, raColor },
            { { rFill, raTop, 0.0f }, { 0, 0, 1 }, raColor },
            { { rFill, raBot, 0.0f }, { 0, 0, 1 }, raColor },

            // LA Fill
            { { aLeft, laBot, 0.0f }, { 0, 0, 1 }, laColor },
            { { aLeft, laTop, 0.0f }, { 0, 0, 1 }, laColor },
            { { lFill, laTop, 0.0f }, { 0, 0, 1 }, laColor },
            { { aLeft, laBot, 0.0f }, { 0, 0, 1 }, laColor },
            { { lFill, laTop, 0.0f }, { 0, 0, 1 }, laColor },
            { { lFill, laBot, 0.0f }, { 0, 0, 1 }, laColor },
        };

        if (SUCCEEDED(m_context->Map(m_dynamicAmmoBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, ammoVerts.data(), sizeof(Vertex) * ammoVerts.size());
            m_context->Unmap(m_dynamicAmmoBuffer.Get(), 0);
        }

        m_context->IASetVertexBuffers(0, 1, m_dynamicAmmoBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->Draw(static_cast<UINT>(ammoVerts.size()), 0);
    }

    void D3D11Renderer::RenderVREye(
        const XMMATRIX& view,
        const XMMATRIX& proj,
        ID3D11RenderTargetView* rtv,
        ID3D11DepthStencilView* dsv,
        const D3D11_VIEWPORT& viewport,
        const MechController& mech,
        const WeaponSystem& weapons,
        const std::vector<TargetDummy>& targets,
        const TargetLockSystem& lockSystem,
        bool isLeftEye)
    {
        if (!rtv || !dsv) return;

        // Clear VR eye target
        const float clearColor[4] = { 0.06f, 0.07f, 0.10f, 1.0f };
        m_context->ClearRenderTargetView(rtv, clearColor);
        m_context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        m_context->RSSetViewports(1, &viewport);
        m_context->RSSetState(m_rasterizerState.Get());
        m_context->OMSetRenderTargets(1, &rtv, dsv);
        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

        // 1. Render Grid Floor & Hangar Walls
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

        // 2. Render Target Dummies
        RenderTargetDummies(targets, view, proj);

        // 3. Render Mech (VR is always first-person cockpit, so isFPV=true draws only arms/weapons)
        RenderMech(mech, view, proj, true);

        // 4. Render Projectiles
        RenderProjectiles(weapons, view, proj);
    }

    void D3D11Renderer::EndFrame(bool vsync)
    {
        m_swapChain->Present(vsync ? 1 : 0, 0);
    }
}
