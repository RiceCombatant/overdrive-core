#include "D3D11Renderer.hpp"
#include "combat/TargetLockSystem.hpp"
#include "combat/AIBotMech.hpp"
#include "network/NetworkManager.hpp"
#include "ui/MenuSystem.hpp"
#include "core/InternalSystem.hpp"
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <iostream>
#include <cmath>
#include <string>
#include <cctype>

namespace Overdrive
{
    namespace
    {
        // Vector stroke font for HUD & Cyber Menu display
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

            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

            switch (c)
            {
            // Digits 0-9
            case '0':
                addL(l, t, r, t); addL(r, t, r, b); addL(r, b, l, b); addL(l, b, l, t); addL(l, b, r, t);
                break;
            case '1':
                addL(cx, t, r, t); addL(r, t, r, b); addL(l, b, r, b);
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

            // Letters A-Z
            case 'A':
                addL(l, b, l, t); addL(l, t, r, t); addL(r, t, r, b); addL(l, m, r, m);
                break;
            case 'B':
                addL(l, b, l, t); addL(l, t, r, t); addL(r, t, r, m); addL(r, m, l, m); addL(r, m, r, b); addL(r, b, l, b);
                break;
            case 'C':
                addL(r, t, l, t); addL(l, t, l, b); addL(l, b, r, b);
                break;
            case 'D':
                addL(l, b, l, t); addL(l, t, cx, t); addL(cx, t, r, m); addL(r, m, cx, b); addL(cx, b, l, b);
                break;
            case 'E':
                addL(r, t, l, t); addL(l, t, l, b); addL(l, b, r, b); addL(l, m, cx, m);
                break;
            case 'F':
                addL(l, b, l, t); addL(l, t, r, t); addL(l, m, cx, m);
                break;
            case 'G':
                addL(r, t, l, t); addL(l, t, l, b); addL(l, b, r, b); addL(r, b, r, m); addL(r, m, cx, m);
                break;
            case 'H':
                addL(l, b, l, t); addL(r, b, r, t); addL(l, m, r, m);
                break;
            case 'I':
                addL(cx, t, cx, b); addL(l, t, r, t); addL(l, b, r, b);
                break;
            case 'J':
                addL(r, t, r, b); addL(r, b, l, b); addL(l, b, l, m);
                break;
            case 'K':
                addL(l, b, l, t); addL(r, t, l, m); addL(l, m, r, b);
                break;
            case 'L':
                addL(l, t, l, b); addL(l, b, r, b);
                break;
            case 'M':
                addL(l, b, l, t); addL(l, t, cx, m); addL(cx, m, r, t); addL(r, t, r, b);
                break;
            case 'N':
                addL(l, b, l, t); addL(l, t, r, b); addL(r, b, r, t);
                break;
            case 'O':
                addL(l, t, r, t); addL(r, t, r, b); addL(r, b, l, b); addL(l, b, l, t);
                break;
            case 'P':
                addL(l, b, l, t); addL(l, t, r, t); addL(r, t, r, m); addL(r, m, l, m);
                break;
            case 'Q':
                addL(l, t, r, t); addL(r, t, r, b); addL(r, b, l, b); addL(l, b, l, t); addL(cx, m, r, b);
                break;
            case 'R':
                addL(l, b, l, t); addL(l, t, r, t); addL(r, t, r, m); addL(r, m, l, m); addL(cx, m, r, b);
                break;
            case 'S':
                addL(r, t, l, t); addL(l, t, l, m); addL(l, m, r, m); addL(r, m, r, b); addL(r, b, l, b);
                break;
            case 'T':
                addL(l, t, r, t); addL(cx, t, cx, b);
                break;
            case 'U':
                addL(l, t, l, b); addL(l, b, r, b); addL(r, b, r, t);
                break;
            case 'V':
                addL(l, t, cx, b); addL(cx, b, r, t);
                break;
            case 'W':
                addL(l, t, l, b); addL(l, b, cx, m); addL(cx, m, r, b); addL(r, b, r, t);
                break;
            case 'X':
                addL(l, t, r, b); addL(r, t, l, b);
                break;
            case 'Y':
                addL(l, t, cx, m); addL(r, t, cx, m); addL(cx, m, cx, b);
                break;
            case 'Z':
                addL(l, t, r, t); addL(r, t, l, b); addL(l, b, r, b);
                break;

            // Symbols
            case '[':
                addL(r, t, l, t); addL(l, t, l, b); addL(l, b, r, b);
                break;
            case ']':
                addL(l, t, r, t); addL(r, t, r, b); addL(r, b, l, b);
                break;
            case '(':
                addL(r, t, l, m); addL(l, m, r, b);
                break;
            case ')':
                addL(l, t, r, m); addL(r, m, l, b);
                break;
            case '-':
                addL(l, m, r, m);
                break;
            case '_':
                addL(l, b, r, b);
                break;
            case '/':
                addL(l, b, r, t);
                break;
            case '\\':
                addL(l, t, r, b);
                break;
            case ':':
                addL(cx, m + h * 0.20f, cx, m + h * 0.28f);
                addL(cx, m - h * 0.28f, cx, m - h * 0.20f);
                break;
            case '.':
                addL(cx, b, cx, b + h * 0.12f);
                break;
            case '>':
                addL(l, t, r, m); addL(r, m, l, b);
                break;
            case '<':
                addL(r, t, l, m); addL(l, m, r, b);
                break;
            case '+':
                addL(l, m, r, m); addL(cx, t, cx, b);
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

    bool D3D11Renderer::Initialize(HWND hwnd, int width, int height, const LUID* preferredLuid)
    {
        m_hwnd = hwnd;
        m_width = width;
        m_height = height;

        if (!CreateDeviceAndSwapChain(hwnd, preferredLuid)) return false;
        if (!CreateRenderTargetAndDepthBuffer()) return false;
        if (!InitShadersAndInputLayout()) return false;
        if (!m_gridFloor.Initialize(m_device.Get())) return false;
        if (!InitMechGeometry()) return false;
        if (!InitFrameMeshes()) return false;
        if (!InitWeaponMeshes()) return false;
        if (!InitJetGeometry()) return false;
        if (!InitHUDGeometry()) return false;
        if (!InitCombatGeometry()) return false;

        return true;
    }

    bool D3D11Renderer::CreateDeviceAndSwapChain(HWND hwnd, const LUID* preferredLuid)
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

        // Try to match preferred LUID (e.g. GPU adapter requested by OpenXR/Meta Quest)
        ComPtr<IDXGIAdapter1> chosenAdapter;
        if (preferredLuid && (preferredLuid->LowPart != 0 || preferredLuid->HighPart != 0))
        {
            ComPtr<IDXGIFactory1> dxgiFactory;
            if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgiFactory.GetAddressOf()))))
            {
                for (UINT i = 0; ; ++i)
                {
                    ComPtr<IDXGIAdapter1> adapter;
                    if (FAILED(dxgiFactory->EnumAdapters1(i, adapter.GetAddressOf()))) break;
                    DXGI_ADAPTER_DESC1 desc;
                    adapter->GetDesc1(&desc);
                    if (memcmp(&desc.AdapterLuid, preferredLuid, sizeof(LUID)) == 0)
                    {
                        chosenAdapter = adapter;
                        std::wcout << L"[D3D11] Matching OpenXR GPU Adapter Selected: " << desc.Description << std::endl;
                        break;
                    }
                }
            }
        }

        D3D_DRIVER_TYPE driverType = chosenAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;

        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            chosenAdapter.Get(),
            driverType,
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
                chosenAdapter.Get(),
                driverType,
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

        // Depth Stencil State (Thruster Plume - Depth test enabled, depth write disabled)
        D3D11_DEPTH_STENCIL_DESC thrusterDsDesc = dsDesc;
        thrusterDsDesc.DepthEnable = TRUE;
        thrusterDsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        thrusterDsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

        hr = m_device->CreateDepthStencilState(&thrusterDsDesc, m_thrusterDepthState.GetAddressOf());
        if (FAILED(hr)) return false;

        // Additive Blend State for Glowing Thruster Plumes
        D3D11_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        hr = m_device->CreateBlendState(&blendDesc, m_additiveBlendState.GetAddressOf());
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

        // 3. Forearms & Hands (Weapons are dynamically mounted per slot)
        addBox(-0.85f, 1.3f, 0.2f, 0.3f, 0.7f, 0.35f, darkFrame);
        addBox( 0.85f, 1.3f, 0.2f, 0.3f, 0.7f, 0.35f, darkFrame);

        // 4. Reverse-joint Legs
        addBox(-0.35f, 0.65f, 0.0f, 0.3f, 1.3f, 0.4f, mainPink);
        addBox( 0.35f, 0.65f, 0.0f, 0.3f, 1.3f, 0.4f, mainPink);
        addBox(-0.35f, 0.1f, 0.15f, 0.35f, 0.2f, 0.7f, darkFrame);
        addBox( 0.35f, 0.1f, 0.15f, 0.35f, 0.2f, 0.7f, darkFrame);

        // 5. Back Thrusters
        addBox(-0.35f, 1.6f, -0.45f, 0.22f, 0.22f, 0.2f, thrusterCyan);
        addBox( 0.35f, 1.6f, -0.45f, 0.22f, 0.22f, 0.2f, thrusterCyan);

        // 6. Back Weapon Hanger Racks (Pylons & Mount Cradles)
        addBox(-0.78f, 2.10f, -0.30f, 0.14f, 0.38f, 0.18f, darkFrame);
        addBox(-0.80f, 2.26f, -0.38f, 0.22f, 0.14f, 0.26f, whitePlate);
        addBox( 0.78f, 2.10f, -0.30f, 0.14f, 0.38f, 0.18f, darkFrame);
        addBox( 0.80f, 2.26f, -0.38f, 0.22f, 0.14f, 0.26f, whitePlate);

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
        if (FAILED(hr)) return false;

        // Create Mech geometry buffers for 4 player color themes (Slot 0 to 3)
        struct PlayerColorTheme {
            XMFLOAT4 primary;
            XMFLOAT4 frame;
            XMFLOAT4 armor;
            XMFLOAT4 thruster;
        };

        PlayerColorTheme themes[4] = {
            // Player 1 / Slot 0 (Host - Crimson / Dark Titanium)
            { { 0.82f, 0.12f, 0.18f, 1.0f }, { 0.16f, 0.17f, 0.20f, 1.0f }, { 0.75f, 0.78f, 0.82f, 1.0f }, { 1.00f, 0.45f, 0.10f, 1.0f } },
            // Player 2 / Slot 1 (Cobalt / Silver)
            { { 0.15f, 0.50f, 0.95f, 1.0f }, { 0.12f, 0.14f, 0.20f, 1.0f }, { 0.88f, 0.90f, 0.95f, 1.0f }, { 0.20f, 0.90f, 1.00f, 1.0f } },
            // Player 3 / Slot 2 (Amber / Charcoal)
            { { 0.95f, 0.65f, 0.10f, 1.0f }, { 0.16f, 0.16f, 0.18f, 1.0f }, { 0.70f, 0.68f, 0.65f, 1.0f }, { 1.00f, 0.85f, 0.15f, 1.0f } },
            // Player 4 / Slot 3 (Emerald / Violet)
            { { 0.15f, 0.85f, 0.45f, 1.0f }, { 0.20f, 0.15f, 0.25f, 1.0f }, { 0.80f, 0.85f, 0.82f, 1.0f }, { 0.30f, 1.00f, 0.60f, 1.0f } }
        };

        for (int p = 0; p < 4; ++p)
        {
            std::vector<Vertex> pVertices;
            const auto& th = themes[p];

            auto addPBox = [&](float cx, float cy, float cz, float sx, float sy, float sz, XMFLOAT4 col) {
                float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;

                pVertices.push_back({ { cx - hx, cy - hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });
                pVertices.push_back({ { cx - hx, cy + hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });
                pVertices.push_back({ { cx + hx, cy + hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });
                pVertices.push_back({ { cx + hx, cy - hy, cz - hz }, { 0.0f, 0.0f, -1.0f }, col });

                pVertices.push_back({ { cx + hx, cy - hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });
                pVertices.push_back({ { cx + hx, cy + hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });
                pVertices.push_back({ { cx - hx, cy + hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });
                pVertices.push_back({ { cx - hx, cy - hy, cz + hz }, { 0.0f, 0.0f, 1.0f }, col });
            };

            // 1. Torso & Core
            addPBox(0.0f, 1.6f, 0.0f, 0.9f, 0.8f, 0.7f, th.frame);
            addPBox(0.0f, 1.65f, 0.2f, 0.75f, 0.65f, 0.5f, th.primary);
            addPBox(0.0f, 1.85f, 0.15f, 0.45f, 0.35f, 0.4f, th.armor);

            // 2. Left & Right Shoulder Boosters
            addPBox(-0.75f, 1.85f, -0.1f, 0.35f, 0.9f, 0.6f, th.primary);
            addPBox( 0.75f, 1.85f, -0.1f, 0.35f, 0.9f, 0.6f, th.primary);
            addPBox(-0.80f, 2.05f, -0.2f, 0.25f, 0.6f, 0.4f, th.frame);
            addPBox( 0.80f, 2.05f, -0.2f, 0.25f, 0.6f, 0.4f, th.frame);

            // 3. Forearms & Hands
            addPBox(-0.85f, 1.3f, 0.2f, 0.3f, 0.7f, 0.35f, th.frame);
            addPBox( 0.85f, 1.3f, 0.2f, 0.3f, 0.7f, 0.35f, th.frame);

            // 4. Reverse-joint Legs
            addPBox(-0.35f, 0.65f, 0.0f, 0.3f, 1.3f, 0.4f, th.primary);
            addPBox( 0.35f, 0.65f, 0.0f, 0.3f, 1.3f, 0.4f, th.primary);
            addPBox(-0.35f, 0.1f, 0.15f, 0.35f, 0.2f, 0.7f, th.frame);
            addPBox( 0.35f, 0.1f, 0.15f, 0.35f, 0.2f, 0.7f, th.frame);

            // 5. Back Thrusters
            addPBox(-0.35f, 1.6f, -0.45f, 0.22f, 0.22f, 0.2f, th.thruster);
            addPBox( 0.35f, 1.6f, -0.45f, 0.22f, 0.22f, 0.2f, th.thruster);

            // 6. Back Weapon Hanger Racks
            addPBox(-0.78f, 2.10f, -0.30f, 0.14f, 0.38f, 0.18f, th.frame);
            addPBox(-0.80f, 2.26f, -0.38f, 0.22f, 0.14f, 0.26f, th.armor);
            addPBox( 0.78f, 2.10f, -0.30f, 0.14f, 0.38f, 0.18f, th.frame);
            addPBox( 0.80f, 2.26f, -0.38f, 0.22f, 0.14f, 0.26f, th.armor);

            D3D11_BUFFER_DESC pvbd = {};
            pvbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * pVertices.size());
            pvbd.Usage = D3D11_USAGE_IMMUTABLE;
            pvbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA pvdata = {};
            pvdata.pSysMem = pVertices.data();

            hr = m_device->CreateBuffer(&pvbd, &pvdata, m_playerMechVertexBuffer[p].GetAddressOf());
            if (FAILED(hr)) return false;
        }

        m_enemyMechVertexBuffer = m_playerMechVertexBuffer[0];
        return true;
    }

    bool D3D11Renderer::InitFrameMeshes()
    {
        auto buildMesh = [&](const std::function<void(std::vector<Vertex>&, std::vector<uint32_t>&)>& builder, FramePartType type) -> bool {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            builder(vertices, indices);

            FrameMesh fm;
            fm.indexCount = static_cast<UINT>(indices.size());

            D3D11_BUFFER_DESC vbd = {};
            vbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
            vbd.Usage = D3D11_USAGE_IMMUTABLE;
            vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA vdata = {};
            vdata.pSysMem = vertices.data();

            HRESULT hr = m_device->CreateBuffer(&vbd, &vdata, fm.vertexBuffer.GetAddressOf());
            if (FAILED(hr)) return false;

            D3D11_BUFFER_DESC ibd = {};
            ibd.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
            ibd.Usage = D3D11_USAGE_IMMUTABLE;
            ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA idata = {};
            idata.pSysMem = indices.data();

            hr = m_device->CreateBuffer(&ibd, &idata, fm.indexBuffer.GetAddressOf());
            if (FAILED(hr)) return false;

            m_frameMeshes[type] = std::move(fm);
            return true;
        };

        auto addBox = [](std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, float cx, float cy, float cz, float sx, float sy, float sz, XMFLOAT4 col) {
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

        // Palettes for various frame styles
        XMFLOAT4 mainPink    = { 0.85f, 0.28f, 0.48f, 1.0f };
        XMFLOAT4 whitePlate  = { 0.92f, 0.94f, 0.96f, 1.0f };
        XMFLOAT4 darkFrame   = { 0.18f, 0.20f, 0.24f, 1.0f };
        XMFLOAT4 darkTitan   = { 0.14f, 0.15f, 0.18f, 1.0f };
        XMFLOAT4 crimsonRed  = { 0.82f, 0.15f, 0.18f, 1.0f };
        XMFLOAT4 aeroCyan    = { 0.25f, 0.75f, 0.95f, 1.0f };
        XMFLOAT4 goldAmber   = { 0.95f, 0.72f, 0.15f, 1.0f };
        XMFLOAT4 cyanVisor   = { 0.25f, 0.85f, 1.00f, 1.0f };
        XMFLOAT4 emeraldGlow = { 0.15f, 0.95f, 0.55f, 1.0f };
        XMFLOAT4 redGlow     = { 1.00f, 0.20f, 0.20f, 1.0f };
        XMFLOAT4 amberGlow   = { 1.00f, 0.70f, 0.15f, 1.0f };

        // ====================================================
        // 1. HEAD UNITS (4 Types)
        // ====================================================
        // (A) HD-012 VEGA (Standard Visor)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 1.95f, 0.10f, 0.35f, 0.28f, 0.38f, whitePlate);
            addBox(verts, idxs, 0.0f, 1.95f, 0.28f, 0.28f, 0.10f, 0.08f, cyanVisor);
            addBox(verts, idxs, 0.0f, 1.84f, 0.18f, 0.22f, 0.08f, 0.20f, darkFrame);
            addBox(verts, idxs, 0.18f, 2.12f, 0.05f, 0.03f, 0.35f, 0.03f, darkFrame);
        }, FramePartType::HeadStandard)) return false;

        // (B) HD-099 AERO (Streamline Light)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 1.92f, 0.15f, 0.26f, 0.20f, 0.46f, aeroCyan);
            addBox(verts, idxs, 0.0f, 1.92f, 0.37f, 0.22f, 0.06f, 0.06f, emeraldGlow);
            addBox(verts, idxs, -0.16f, 1.98f, 0.05f, 0.03f, 0.18f, 0.22f, whitePlate);
            addBox(verts, idxs,  0.16f, 1.98f, 0.05f, 0.03f, 0.18f, 0.22f, whitePlate);
        }, FramePartType::HeadLight)) return false;

        // (C) HD-044 TITAN (Heavy Blast-shield)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 1.98f, 0.10f, 0.44f, 0.36f, 0.42f, darkTitan);
            addBox(verts, idxs, 0.0f, 1.98f, 0.30f, 0.26f, 0.08f, 0.06f, redGlow);
            addBox(verts, idxs, 0.0f, 1.86f, 0.28f, 0.36f, 0.14f, 0.16f, crimsonRed);
            addBox(verts, idxs, -0.22f, 2.05f, 0.10f, 0.06f, 0.22f, 0.30f, darkFrame);
            addBox(verts, idxs,  0.22f, 2.05f, 0.10f, 0.06f, 0.22f, 0.30f, darkFrame);
        }, FramePartType::HeadHeavy)) return false;

        // (D) HD-077 PALADIN (Commander Blade Antenna)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 1.96f, 0.12f, 0.32f, 0.28f, 0.38f, goldAmber);
            addBox(verts, idxs, -0.10f, 1.96f, 0.28f, 0.08f, 0.08f, 0.06f, amberGlow);
            addBox(verts, idxs,  0.10f, 1.96f, 0.28f, 0.08f, 0.08f, 0.06f, amberGlow);
            // High blade crest antenna
            addBox(verts, idxs, 0.0f, 2.25f, 0.18f, 0.04f, 0.42f, 0.14f, whitePlate);
        }, FramePartType::HeadPaladin)) return false;

        // ====================================================
        // 2. CORE UNITS (4 Types)
        // ====================================================
        // (A) CR-020 ORBITER (Standard Core)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 1.60f, 0.0f, 0.90f, 0.80f, 0.70f, darkFrame);
            addBox(verts, idxs, 0.0f, 1.65f, 0.20f, 0.75f, 0.65f, 0.50f, whitePlate);
            addBox(verts, idxs, 0.0f, 1.85f, 0.15f, 0.45f, 0.35f, 0.40f, mainPink);
            addBox(verts, idxs, -0.35f, 1.60f, -0.45f, 0.22f, 0.22f, 0.20f, cyanVisor);
            addBox(verts, idxs,  0.35f, 1.60f, -0.45f, 0.22f, 0.22f, 0.20f, cyanVisor);
        }, FramePartType::CoreStandard)) return false;

        // (B) CR-088 PHANTOM (Lightweight Stream Core)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 1.60f, 0.0f, 0.76f, 0.75f, 0.65f, darkFrame);
            addBox(verts, idxs, 0.0f, 1.62f, 0.25f, 0.60f, 0.58f, 0.48f, aeroCyan);
            addBox(verts, idxs, -0.46f, 1.55f, 0.10f, 0.16f, 0.40f, 0.35f, whitePlate);
            addBox(verts, idxs,  0.46f, 1.55f, 0.10f, 0.16f, 0.40f, 0.35f, whitePlate);
            addBox(verts, idxs, -0.30f, 1.65f, -0.42f, 0.18f, 0.20f, 0.22f, cyanVisor);
            addBox(verts, idxs,  0.30f, 1.65f, -0.42f, 0.18f, 0.20f, 0.22f, cyanVisor);
        }, FramePartType::CoreLight)) return false;

        // (C) CR-055 GOLIATH (Dreadnought Bulk Core)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 1.60f, 0.0f, 1.08f, 0.88f, 0.82f, darkTitan);
            addBox(verts, idxs, 0.0f, 1.65f, 0.25f, 0.92f, 0.75f, 0.55f, crimsonRed);
            addBox(verts, idxs, 0.0f, 1.82f, 0.20f, 0.55f, 0.40f, 0.45f, darkFrame);
            // 4-cluster heavy thruster housings
            addBox(verts, idxs, -0.38f, 1.72f, -0.52f, 0.20f, 0.20f, 0.22f, redGlow);
            addBox(verts, idxs,  0.38f, 1.72f, -0.52f, 0.20f, 0.20f, 0.22f, redGlow);
            addBox(verts, idxs, -0.38f, 1.48f, -0.52f, 0.20f, 0.20f, 0.22f, redGlow);
            addBox(verts, idxs,  0.38f, 1.48f, -0.52f, 0.20f, 0.20f, 0.22f, redGlow);
        }, FramePartType::CoreHeavy)) return false;

        // (D) CR-066 TEMPEST (Overdrive Generator Core)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 1.60f, 0.0f, 0.88f, 0.82f, 0.75f, darkFrame);
            addBox(verts, idxs, 0.0f, 1.65f, 0.22f, 0.70f, 0.60f, 0.52f, goldAmber);
            addBox(verts, idxs, -0.52f, 1.62f, -0.05f, 0.12f, 0.65f, 0.40f, amberGlow);
            addBox(verts, idxs,  0.52f, 1.62f, -0.05f, 0.12f, 0.65f, 0.40f, amberGlow);
            addBox(verts, idxs, -0.35f, 1.60f, -0.48f, 0.26f, 0.26f, 0.24f, amberGlow);
            addBox(verts, idxs,  0.35f, 1.60f, -0.48f, 0.26f, 0.26f, 0.24f, amberGlow);
        }, FramePartType::CoreTempest)) return false;

        // ====================================================
        // 3. ARMS UNITS (4 Types)
        // ====================================================
        // (A) AM-030 STRIKER (Standard Arms)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            // Shoulder Armor & Boosters
            addBox(verts, idxs, -0.75f, 1.85f, -0.10f, 0.35f, 0.90f, 0.60f, mainPink);
            addBox(verts, idxs,  0.75f, 1.85f, -0.10f, 0.35f, 0.90f, 0.60f, mainPink);
            addBox(verts, idxs, -0.80f, 2.05f, -0.20f, 0.25f, 0.60f, 0.40f, whitePlate);
            addBox(verts, idxs,  0.80f, 2.05f, -0.20f, 0.25f, 0.60f, 0.40f, whitePlate);
            // Forearms & Hands
            addBox(verts, idxs, -0.85f, 1.30f, 0.20f, 0.30f, 0.70f, 0.35f, darkFrame);
            addBox(verts, idxs,  0.85f, 1.30f, 0.20f, 0.30f, 0.70f, 0.35f, darkFrame);
            // Back Weapon Hanger Racks
            addBox(verts, idxs, -0.78f, 2.10f, -0.30f, 0.14f, 0.38f, 0.18f, darkFrame);
            addBox(verts, idxs, -0.80f, 2.26f, -0.38f, 0.22f, 0.14f, 0.26f, whitePlate);
            addBox(verts, idxs,  0.78f, 2.10f, -0.30f, 0.14f, 0.38f, 0.18f, darkFrame);
            addBox(verts, idxs,  0.80f, 2.26f, -0.38f, 0.22f, 0.14f, 0.26f, whitePlate);
        }, FramePartType::ArmsStandard)) return false;

        // (B) AM-077 VELOX (Light Fast Servo Arms)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            // Swept-wing aeroshoulders
            addBox(verts, idxs, -0.72f, 1.82f, -0.08f, 0.26f, 0.75f, 0.52f, aeroCyan);
            addBox(verts, idxs,  0.72f, 1.82f, -0.08f, 0.26f, 0.75f, 0.52f, aeroCyan);
            addBox(verts, idxs, -0.88f, 1.88f, -0.12f, 0.08f, 0.45f, 0.38f, whitePlate);
            addBox(verts, idxs,  0.88f, 1.88f, -0.12f, 0.08f, 0.45f, 0.38f, whitePlate);
            // Slim servo forearms
            addBox(verts, idxs, -0.80f, 1.30f, 0.20f, 0.22f, 0.68f, 0.26f, darkFrame);
            addBox(verts, idxs,  0.80f, 1.30f, 0.20f, 0.22f, 0.68f, 0.26f, darkFrame);
            // Hanger racks
            addBox(verts, idxs, -0.74f, 2.12f, -0.28f, 0.12f, 0.32f, 0.16f, darkFrame);
            addBox(verts, idxs,  0.74f, 2.12f, -0.28f, 0.12f, 0.32f, 0.16f, darkFrame);
        }, FramePartType::ArmsLight)) return false;

        // (C) AM-066 COLOSSUS (Heavy Shield Arms)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            // Massive bulk shield shoulders
            addBox(verts, idxs, -0.85f, 1.90f, -0.05f, 0.48f, 1.05f, 0.72f, crimsonRed);
            addBox(verts, idxs,  0.85f, 1.90f, -0.05f, 0.48f, 1.05f, 0.72f, crimsonRed);
            addBox(verts, idxs, -1.10f, 1.85f, -0.05f, 0.08f, 0.75f, 0.50f, darkTitan);
            addBox(verts, idxs,  1.10f, 1.85f, -0.05f, 0.08f, 0.75f, 0.50f, darkTitan);
            // Thick hydraulic forearms
            addBox(verts, idxs, -0.92f, 1.32f, 0.22f, 0.38f, 0.75f, 0.42f, darkTitan);
            addBox(verts, idxs,  0.92f, 1.32f, 0.22f, 0.38f, 0.75f, 0.42f, darkTitan);
            // Heavy weapon racks
            addBox(verts, idxs, -0.82f, 2.15f, -0.32f, 0.18f, 0.42f, 0.22f, darkFrame);
            addBox(verts, idxs,  0.82f, 2.15f, -0.32f, 0.18f, 0.42f, 0.22f, darkFrame);
        }, FramePartType::ArmsHeavy)) return false;

        // (D) AM-088 VALIANT (Melee Combat Arms)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, -0.78f, 1.88f, -0.08f, 0.38f, 0.88f, 0.62f, goldAmber);
            addBox(verts, idxs,  0.78f, 1.88f, -0.08f, 0.38f, 0.88f, 0.62f, goldAmber);
            // Shoulder strike blades
            addBox(verts, idxs, -1.02f, 2.05f, 0.05f, 0.12f, 0.15f, 0.32f, whitePlate);
            addBox(verts, idxs,  1.02f, 2.05f, 0.05f, 0.12f, 0.15f, 0.32f, whitePlate);
            // Forearms with integrated thrusters
            addBox(verts, idxs, -0.86f, 1.30f, 0.20f, 0.32f, 0.70f, 0.36f, darkFrame);
            addBox(verts, idxs,  0.86f, 1.30f, 0.20f, 0.32f, 0.70f, 0.36f, darkFrame);
            addBox(verts, idxs, -0.92f, 1.25f, -0.02f, 0.14f, 0.32f, 0.16f, amberGlow);
            addBox(verts, idxs,  0.92f, 1.25f, -0.02f, 0.14f, 0.32f, 0.16f, amberGlow);
            // Hanger racks
            addBox(verts, idxs, -0.78f, 2.10f, -0.30f, 0.14f, 0.38f, 0.18f, darkFrame);
            addBox(verts, idxs,  0.78f, 2.10f, -0.30f, 0.14f, 0.38f, 0.18f, darkFrame);
        }, FramePartType::ArmsValiant)) return false;

        // ====================================================
        // 4. LEGS UNITS (4 Types)
        // ====================================================
        // (A) LG-066 REVERSE-JOINT (AC Style High-jump Reverse Joint)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            // Forward upper thigh
            addBox(verts, idxs, -0.35f, 1.05f, -0.15f, 0.28f, 0.55f, 0.35f, mainPink);
            addBox(verts, idxs,  0.35f, 1.05f, -0.15f, 0.28f, 0.55f, 0.35f, mainPink);
            // Backward-bent reverse knee joint
            addBox(verts, idxs, -0.35f, 0.60f, -0.32f, 0.26f, 0.60f, 0.32f, darkFrame);
            addBox(verts, idxs,  0.35f, 0.60f, -0.32f, 0.26f, 0.60f, 0.32f, darkFrame);
            // Forward return shin
            addBox(verts, idxs, -0.35f, 0.25f, 0.05f, 0.28f, 0.50f, 0.34f, mainPink);
            addBox(verts, idxs,  0.35f, 0.25f, 0.05f, 0.28f, 0.50f, 0.34f, mainPink);
            // Spring damper claw foot
            addBox(verts, idxs, -0.35f, 0.06f, 0.18f, 0.34f, 0.14f, 0.75f, darkFrame);
            addBox(verts, idxs,  0.35f, 0.06f, 0.18f, 0.34f, 0.14f, 0.75f, darkFrame);
        }, FramePartType::LegsReverse)) return false;

        // (B) LG-040 BIPEDAL (Standard Medium Biped)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            // Straight upright thighs
            addBox(verts, idxs, -0.35f, 0.95f, 0.0f, 0.30f, 0.65f, 0.38f, whitePlate);
            addBox(verts, idxs,  0.35f, 0.95f, 0.0f, 0.30f, 0.65f, 0.38f, whitePlate);
            // Knee armor blocks
            addBox(verts, idxs, -0.35f, 0.65f, 0.08f, 0.32f, 0.25f, 0.42f, darkFrame);
            addBox(verts, idxs,  0.35f, 0.65f, 0.08f, 0.32f, 0.25f, 0.42f, darkFrame);
            // Shins
            addBox(verts, idxs, -0.35f, 0.35f, 0.0f, 0.28f, 0.55f, 0.36f, whitePlate);
            addBox(verts, idxs,  0.35f, 0.35f, 0.0f, 0.28f, 0.55f, 0.36f, whitePlate);
            // Solid foot sole
            addBox(verts, idxs, -0.35f, 0.08f, 0.10f, 0.34f, 0.16f, 0.65f, darkFrame);
            addBox(verts, idxs,  0.35f, 0.08f, 0.10f, 0.34f, 0.16f, 0.65f, darkFrame);
        }, FramePartType::LegsBipedal)) return false;

        // (C) LG-088 QUADRUPED / HEAVY (Heavy Fortress Quad Legs)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            // Massive heavy thighs
            addBox(verts, idxs, -0.42f, 0.90f, 0.0f, 0.44f, 0.70f, 0.50f, darkTitan);
            addBox(verts, idxs,  0.42f, 0.90f, 0.0f, 0.44f, 0.70f, 0.50f, darkTitan);
            // Reinforced knee bulwarks
            addBox(verts, idxs, -0.42f, 0.55f, 0.12f, 0.46f, 0.35f, 0.55f, crimsonRed);
            addBox(verts, idxs,  0.42f, 0.55f, 0.12f, 0.46f, 0.35f, 0.55f, crimsonRed);
            // Heavy fortress boot feet
            addBox(verts, idxs, -0.42f, 0.10f, 0.15f, 0.52f, 0.22f, 0.85f, darkTitan);
            addBox(verts, idxs,  0.42f, 0.10f, 0.15f, 0.52f, 0.22f, 0.85f, darkTitan);
        }, FramePartType::LegsHeavyQuad)) return false;

        // (D) LG-055 RUNNER (High-mobility Sprint Legs)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            // Aerodynamic legs
            addBox(verts, idxs, -0.34f, 0.92f, 0.0f, 0.26f, 0.65f, 0.34f, aeroCyan);
            addBox(verts, idxs,  0.34f, 0.92f, 0.0f, 0.26f, 0.65f, 0.34f, aeroCyan);
            // Lateral hover thruster fins
            addBox(verts, idxs, -0.48f, 0.45f, -0.10f, 0.12f, 0.35f, 0.28f, amberGlow);
            addBox(verts, idxs,  0.48f, 0.45f, -0.10f, 0.12f, 0.35f, 0.28f, amberGlow);
            // Speed boots
            addBox(verts, idxs, -0.34f, 0.08f, 0.12f, 0.30f, 0.15f, 0.68f, darkFrame);
            addBox(verts, idxs,  0.34f, 0.08f, 0.12f, 0.30f, 0.15f, 0.68f, darkFrame);
        }, FramePartType::LegsSprint)) return false;

        return true;
    }

    bool D3D11Renderer::InitWeaponMeshes()
    {
        auto buildMesh = [&](const std::function<void(std::vector<Vertex>&, std::vector<uint32_t>&)>& builder, WeaponType type) -> bool {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            builder(vertices, indices);

            WeaponMesh wm;
            wm.indexCount = static_cast<UINT>(indices.size());

            D3D11_BUFFER_DESC vbd = {};
            vbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
            vbd.Usage = D3D11_USAGE_IMMUTABLE;
            vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA vdata = {};
            vdata.pSysMem = vertices.data();

            HRESULT hr = m_device->CreateBuffer(&vbd, &vdata, wm.vertexBuffer.GetAddressOf());
            if (FAILED(hr)) return false;

            D3D11_BUFFER_DESC ibd = {};
            ibd.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
            ibd.Usage = D3D11_USAGE_IMMUTABLE;
            ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA idata = {};
            idata.pSysMem = indices.data();

            hr = m_device->CreateBuffer(&ibd, &idata, wm.indexBuffer.GetAddressOf());
            if (FAILED(hr)) return false;

            m_weaponMeshes[type] = std::move(wm);
            return true;
        };

        XMFLOAT4 darkFrame   = { 0.18f, 0.20f, 0.24f, 1.0f };
        XMFLOAT4 whitePlate  = { 0.92f, 0.94f, 0.96f, 1.0f };
        XMFLOAT4 cyanGlow    = { 0.25f, 0.85f, 1.00f, 1.0f };
        XMFLOAT4 orangeGlow  = { 1.00f, 0.55f, 0.15f, 1.0f };
        XMFLOAT4 purpleFrame = { 0.35f, 0.20f, 0.48f, 1.0f };
        XMFLOAT4 violetGlow  = { 0.88f, 0.20f, 1.00f, 1.0f };
        XMFLOAT4 tanFrame    = { 0.45f, 0.42f, 0.38f, 1.0f };
        XMFLOAT4 amberGlow   = { 1.00f, 0.82f, 0.18f, 1.0f };

        auto addBox = [](std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                         float cx, float cy, float cz, float sx, float sy, float sz, XMFLOAT4 col) {
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

        // 1. Kinetic Rifle (RF-024)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 0.0f, 0.0f, 0.18f, 0.24f, 0.65f, whitePlate);
            addBox(verts, idxs, 0.0f, 0.14f, 0.05f, 0.08f, 0.04f, 0.50f, darkFrame);
            addBox(verts, idxs, 0.0f, 0.03f, 0.65f, 0.07f, 0.07f, 0.70f, darkFrame);
            addBox(verts, idxs, 0.0f, 0.03f, 1.02f, 0.09f, 0.09f, 0.08f, orangeGlow);
            addBox(verts, idxs, 0.0f, -0.18f, -0.05f, 0.09f, 0.22f, 0.18f, darkFrame);
        }, WeaponType::KineticRifle)) return false;

        // 2. Beam Rifle (EN-010)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 0.0f, 0.0f, 0.20f, 0.22f, 0.70f, darkFrame);
            addBox(verts, idxs, 0.0f, 0.02f, 0.52f, 0.13f, 0.13f, 0.65f, cyanGlow);
            addBox(verts, idxs, 0.0f, 0.02f, 0.90f, 0.15f, 0.08f, 0.12f, whitePlate);
            addBox(verts, idxs, 0.0f, -0.12f, 0.15f, 0.04f, 0.16f, 0.35f, cyanGlow);
        }, WeaponType::BeamRifle)) return false;

        // 3. Heavy Plasma (PL-040)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 0.0f, 0.0f, 0.26f, 0.28f, 0.80f, purpleFrame);
            addBox(verts, idxs, 0.0f, 0.0f, 0.0f, 0.16f, 0.16f, 0.82f, violetGlow);
            addBox(verts, idxs, 0.0f, 0.0f, 0.65f, 0.22f, 0.22f, 0.60f, darkFrame);
            addBox(verts, idxs, 0.0f, 0.17f, -0.05f, 0.20f, 0.10f, 0.55f, violetGlow);
        }, WeaponType::HeavyPlasma)) return false;

        // 4. Burst Gun (MG-014)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            addBox(verts, idxs, 0.0f, 0.0f, 0.0f, 0.16f, 0.20f, 0.50f, tanFrame);
            addBox(verts, idxs, 0.0f, 0.02f, 0.42f, 0.11f, 0.11f, 0.45f, darkFrame);
            addBox(verts, idxs, 0.0f, 0.02f, 0.68f, 0.12f, 0.12f, 0.08f, amberGlow);
            addBox(verts, idxs, 0.0f, -0.16f, -0.05f, 0.24f, 0.18f, 0.18f, darkFrame);
        }, WeaponType::BurstGun)) return false;

        // 5. Vertical Micro-Missile Pod (ML-080 VERTI-MISSILE)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            XMFLOAT4 missileWhite = { 0.95f, 0.95f, 0.98f, 1.0f };
            XMFLOAT4 missileRed   = { 0.85f, 0.20f, 0.15f, 1.0f };
            // Main pod armored container
            addBox(verts, idxs, 0.0f, 0.0f, 0.0f, 0.38f, 0.38f, 0.78f, darkFrame);
            addBox(verts, idxs, 0.0f, 0.08f, 0.05f, 0.40f, 0.12f, 0.65f, whitePlate);
            // 4 missile cell launch hatches (2x2)
            addBox(verts, idxs, -0.09f,  0.09f, 0.40f, 0.11f, 0.11f, 0.06f, missileRed);
            addBox(verts, idxs,  0.09f,  0.09f, 0.40f, 0.11f, 0.11f, 0.06f, missileRed);
            addBox(verts, idxs, -0.09f, -0.09f, 0.40f, 0.11f, 0.11f, 0.06f, missileRed);
            addBox(verts, idxs,  0.09f, -0.09f, 0.40f, 0.11f, 0.11f, 0.06f, missileRed);
            // Rear exhaust duct
            addBox(verts, idxs, 0.0f, 0.0f, -0.40f, 0.32f, 0.32f, 0.06f, amberGlow);
        }, WeaponType::MissilePod)) return false;

        // 6. Heavy Bazooka (BZ-033 TITAN)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            // Giant heavy cannon barrel
            addBox(verts, idxs, 0.0f, 0.0f, 0.0f, 0.28f, 0.32f, 0.70f, darkFrame);
            addBox(verts, idxs, 0.0f, 0.06f, 0.70f, 0.24f, 0.24f, 1.15f, darkFrame);
            // Heavy muzzle brake at tip
            addBox(verts, idxs, 0.0f, 0.06f, 1.28f, 0.32f, 0.32f, 0.16f, orangeGlow);
            // Top targeting scope & sensor
            addBox(verts, idxs, 0.0f, 0.24f, 0.15f, 0.12f, 0.12f, 0.38f, cyanGlow);
            // Shoulder recoil stock
            addBox(verts, idxs, 0.0f, -0.05f, -0.45f, 0.22f, 0.26f, 0.35f, whitePlate);
        }, WeaponType::HeavyBazooka)) return false;

        // 7. Laser Blade (LB-077 MOONLIGHT)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            XMFLOAT4 bladeEmitter = { 0.18f, 0.28f, 0.24f, 1.0f };
            XMFLOAT4 moonlightCol = { 0.20f, 0.95f, 0.65f, 1.0f };
            // Forearm emitter housing & buckler
            addBox(verts, idxs, 0.0f, 0.0f, 0.0f, 0.24f, 0.22f, 0.50f, bladeEmitter);
            addBox(verts, idxs, 0.0f, 0.14f, 0.05f, 0.26f, 0.08f, 0.40f, whitePlate);
            // Plasma containment focal ring
            addBox(verts, idxs, 0.0f, 0.0f, 0.30f, 0.20f, 0.18f, 0.12f, moonlightCol);
            // Folding blade stabilization spar
            addBox(verts, idxs, 0.0f, -0.12f, 0.18f, 0.06f, 0.18f, 0.45f, darkFrame);
        }, WeaponType::LaserBlade)) return false;

        // 8. Heavy Gatling Gun (GT-090 VULCAN)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            // Massive cylindrical ammo drum
            addBox(verts, idxs, 0.0f, -0.15f, -0.05f, 0.34f, 0.34f, 0.36f, darkFrame);
            // Motor & feed mechanism
            addBox(verts, idxs, 0.0f, 0.08f, 0.10f, 0.22f, 0.22f, 0.50f, tanFrame);
            // 6-barrel cluster housing & central spindle
            addBox(verts, idxs, 0.0f, 0.08f, 0.70f, 0.10f, 0.10f, 0.85f, darkFrame);
            addBox(verts, idxs, 0.0f, 0.08f, 0.50f, 0.24f, 0.24f, 0.06f, amberGlow);
            addBox(verts, idxs, 0.0f, 0.08f, 0.90f, 0.24f, 0.24f, 0.06f, amberGlow);
            addBox(verts, idxs, 0.0f, 0.08f, 1.15f, 0.22f, 0.22f, 0.08f, darkFrame);
            // Outer muzzle ring
            addBox(verts, idxs, 0.0f, 0.18f, 0.70f, 0.06f, 0.06f, 0.80f, darkFrame);
            addBox(verts, idxs, 0.0f, -0.02f, 0.70f, 0.06f, 0.06f, 0.80f, darkFrame);
            addBox(verts, idxs, 0.10f, 0.08f, 0.70f, 0.06f, 0.06f, 0.80f, darkFrame);
            addBox(verts, idxs, -0.10f, 0.08f, 0.70f, 0.06f, 0.06f, 0.80f, darkFrame);
        }, WeaponType::GatlingGun)) return false;

        // 9. Spread Shotgun (SG-020 BREAKER)
        if (!buildMesh([&](auto& verts, auto& idxs) {
            // Receiver & pump-action assembly
            addBox(verts, idxs, 0.0f, 0.0f, 0.0f, 0.20f, 0.24f, 0.48f, darkFrame);
            // Short fat barrel
            addBox(verts, idxs, 0.0f, 0.06f, 0.48f, 0.16f, 0.16f, 0.60f, whitePlate);
            // Under-barrel tubular magazine
            addBox(verts, idxs, 0.0f, -0.08f, 0.40f, 0.12f, 0.12f, 0.50f, darkFrame);
            // Top ribbed heat shield
            addBox(verts, idxs, 0.0f, 0.16f, 0.35f, 0.10f, 0.05f, 0.45f, orangeGlow);
        }, WeaponType::SpreadShotgun)) return false;

        return true;
    }

    bool D3D11Renderer::InitJetGeometry()
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        XMFLOAT4 baseWhite = { 1.0f, 1.0f, 1.0f, 1.0f };
        XMFLOAT4 midPlume  = { 0.7f, 0.9f, 1.0f, 0.6f };
        XMFLOAT4 tipPlume  = { 0.3f, 0.6f, 1.0f, 0.0f }; // Fades out at the tip

        // 1. Base Nozzle Disc (Center vertex + 8 circle rim vertices)
        uint32_t centerIdx = static_cast<uint32_t>(vertices.size());
        vertices.push_back({ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, baseWhite });

        const int segments = 8;
        uint32_t rimStartIdx = static_cast<uint32_t>(vertices.size());
        for (int i = 0; i < segments; ++i)
        {
            float theta = i * (XM_2PI / static_cast<float>(segments));
            float x = std::cos(theta);
            float y = std::sin(theta);
            vertices.push_back({ { x, y, 0.0f }, { 0.0f, 0.0f, 1.0f }, baseWhite });
        }

        // Base disc triangles
        for (int i = 0; i < segments; ++i)
        {
            uint32_t next = (i + 1) % segments;
            indices.push_back(centerIdx);
            indices.push_back(rimStartIdx + next);
            indices.push_back(rimStartIdx + i);
        }

        // 2. Cone Mantle (Outer tapered flame)
        // Mid-waist ring (slight shock contraction at Z = -0.35f, radius 0.75)
        uint32_t midStartIdx = static_cast<uint32_t>(vertices.size());
        for (int i = 0; i < segments; ++i)
        {
            float theta = i * (XM_2PI / static_cast<float>(segments));
            float x = std::cos(theta) * 0.75f;
            float y = std::sin(theta) * 0.75f;
            vertices.push_back({ { x, y, -0.35f }, { 0.0f, 0.0f, -1.0f }, midPlume });
        }

        // Base to Mid ring quads (2 triangles each, double-sided)
        for (int i = 0; i < segments; ++i)
        {
            uint32_t next = (i + 1) % segments;
            uint32_t b0 = rimStartIdx + i;
            uint32_t b1 = rimStartIdx + next;
            uint32_t m0 = midStartIdx + i;
            uint32_t m1 = midStartIdx + next;

            // Front
            indices.push_back(b0); indices.push_back(m0); indices.push_back(b1);
            indices.push_back(b1); indices.push_back(m0); indices.push_back(m1);
            // Back (double-sided visibility)
            indices.push_back(b0); indices.push_back(b1); indices.push_back(m0);
            indices.push_back(b1); indices.push_back(m1); indices.push_back(m0);
        }

        // Tip vertex (Z = -1.0f, radius = 0.0f)
        uint32_t tipIdx = static_cast<uint32_t>(vertices.size());
        vertices.push_back({ { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -1.0f }, tipPlume });

        // Mid ring to Tip triangles (double-sided)
        for (int i = 0; i < segments; ++i)
        {
            uint32_t next = (i + 1) % segments;
            uint32_t m0 = midStartIdx + i;
            uint32_t m1 = midStartIdx + next;

            // Front
            indices.push_back(m0); indices.push_back(tipIdx); indices.push_back(m1);
            // Back
            indices.push_back(m0); indices.push_back(m1); indices.push_back(tipIdx);
        }

        // 3. Central Cross-Quads (High-intensity plasma core streaks)
        auto addCrossQuad = [&](float x0, float y0, float x1, float y1) {
            uint32_t base = static_cast<uint32_t>(vertices.size());
            vertices.push_back({ { x0, y0,  0.00f }, { 0, 0, 1 }, baseWhite });
            vertices.push_back({ { x1, y1,  0.00f }, { 0, 0, 1 }, baseWhite });
            vertices.push_back({ { x1 * 0.6f, y1 * 0.6f, -0.45f }, { 0, 0, 1 }, midPlume });
            vertices.push_back({ { x0 * 0.6f, y0 * 0.6f, -0.45f }, { 0, 0, 1 }, midPlume });
            vertices.push_back({ { 0.0f, 0.0f, -1.0f }, { 0, 0, 1 }, tipPlume });

            // Base quad
            indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 1);
            indices.push_back(base + 0); indices.push_back(base + 3); indices.push_back(base + 2);
            indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
            indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);

            // Tapered triangle to tip
            indices.push_back(base + 3); indices.push_back(base + 4); indices.push_back(base + 2);
            indices.push_back(base + 3); indices.push_back(base + 2); indices.push_back(base + 4);
        };

        // Horizontal streak
        addCrossQuad(-1.0f, 0.0f, 1.0f, 0.0f);
        // Vertical streak
        addCrossQuad(0.0f, -1.0f, 0.0f, 1.0f);

        m_jetIndexCount = static_cast<UINT>(indices.size());

        D3D11_BUFFER_DESC vbd = {};
        vbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertices.size());
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vdata = {};
        vdata.pSysMem = vertices.data();

        HRESULT hr = m_device->CreateBuffer(&vbd, &vdata, m_jetVertexBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        D3D11_BUFFER_DESC ibd = {};
        ibd.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * indices.size());
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA idata = {};
        idata.pSysMem = indices.data();

        hr = m_device->CreateBuffer(&ibd, &idata, m_jetIndexBuffer.GetAddressOf());
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

        // Dynamic Reticle / UI Buffer (Inner reticle, menu UI, text strings, 3D target reticles: 4096 vertices)
        D3D11_BUFFER_DESC dynRetDesc = {};
        dynRetDesc.ByteWidth = sizeof(Vertex) * 4096;
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

        // 2. Dynamic buffer for Ammo & ACS HUD bars (AP + ACS + 4 weapon slots)
        D3D11_BUFFER_DESC ammoDesc = {};
        ammoDesc.ByteWidth = sizeof(Vertex) * 256;
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

    void D3D11Renderer::ShowArenaModeBanner(const std::string& text, float duration)
    {
        m_bannerText = text;
        m_bannerTimer = duration;
        m_bannerMaxDuration = duration;
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
        const TargetLockSystem& lockSystem,
        const std::vector<RemoteMech>* remoteMechs,
        const NetworkManager* network,
        const FrameSystem* frames,
        const std::vector<AIBotMech>* aiBots)
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

        // 3. Render Remote Enemy Mechs
        if (remoteMechs)
        {
            for (const auto& rMech : *remoteMechs)
            {
                RenderEnemyMech(rMech, view, proj);
            }
        }

        // 4. Render Autonomous Combat AI Bots
        if (aiBots)
        {
            RenderAIBots(*aiBots, view, proj);
        }

        // 5. Render 3D Holographic Lock-On Reticles directly on enemy bodies
        RenderTargetReticles(targets, lockSystem, view, proj, camera.GetEyePosition(), remoteMechs, aiBots);

        // 6. Render Mech
        bool isFPV = (camera.GetMode() == CameraMode::FPV);
        RenderMech(mech, weapons, frames, view, proj, isFPV);

        // 7. Render Projectiles (High-energy glowing laser/bullet tracers)
        RenderProjectiles(weapons, view, proj);

        // 8. Render HUD (Reticle, Dynamic Inner Reticle, Damage Indicator, Tactical Radar, Off-Screen Indicators, EN/Ammo/AP Bars, Net Status)
        RenderHUD(mech, weapons, lockSystem, camera.GetMode(), network, remoteMechs, &targets, &camera, aiBots);
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

    void D3D11Renderer::RenderTargetReticles(
        const std::vector<TargetDummy>& targets,
        const TargetLockSystem& lockSystem,
        const XMMATRIX& view,
        const XMMATRIX& proj,
        const XMFLOAT3& eyePos,
        const std::vector<RemoteMech>* remoteMechs,
        const std::vector<AIBotMech>* aiBots)
    {
        m_context->OMSetDepthStencilState(m_hudDepthDisabledState.Get(), 0);

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = XMMatrixIdentity();
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // Emissive unlit
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
        m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        m_context->IASetInputLayout(m_inputLayout.Get());

        const auto& lockInfo = lockSystem.GetCurrentTarget();
        XMVECTOR camPosVec = XMLoadFloat3(&eyePos);

        std::vector<Vertex> retVerts;
        retVerts.reserve(768);

        XMFLOAT4 lockRed     = { 1.0f, 0.18f, 0.15f, 0.95f }; // AC6 Warning Red
        XMFLOAT4 lockAmber   = { 1.0f, 0.85f, 0.22f, 0.95f }; // Locking in progress
        XMFLOAT4 idleCyan    = { 0.35f, 0.75f, 0.95f, 0.65f }; // Unlocked enemy marker
        XMFLOAT4 distWhite   = { 0.92f, 0.92f, 0.96f, 0.90f };
        XMFLOAT4 hpGreen     = { 0.25f, 0.95f, 0.35f, 0.95f };
        XMFLOAT4 hpRed       = { 0.95f, 0.20f, 0.20f, 0.95f };
        XMFLOAT4 hpBg        = { 0.18f, 0.20f, 0.24f, 0.75f };

        for (int i = 0; i < static_cast<int>(targets.size()); ++i)
        {
            const auto& target = targets[i];
            if (!target.IsAlive()) continue;

            XMFLOAT3 pos = target.GetPosition();
            XMVECTOR targetPos = XMVectorSet(pos.x, pos.y + 0.3f, pos.z, 0.0f);

            // Vector from target to camera
            XMVECTOR toCam = camPosVec - targetPos;
            float dist = XMVectorGetX(XMVector3Length(toCam));
            if (dist < 0.5f || dist > 450.0f) continue;
            toCam = XMVector3Normalize(toCam);

            // Billboard coordinate axes (facing camera)
            XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
            XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(worldUp, toCam));
            XMVECTOR upVec = XMVector3Cross(toCam, rightVec);

            bool isCurrentLocked = (lockInfo.hasTarget && !lockInfo.isRemoteMech && lockInfo.targetIndex == i);

            // Dynamic scale: maintain comfortable size at any distance
            float scale = std::clamp(dist * 0.045f, 1.2f, 6.5f);

            auto addWorldPoint = [&](float r, float u) -> XMFLOAT3 {
                XMVECTOR p = targetPos + rightVec * r + upVec * u;
                XMFLOAT3 outP;
                XMStoreFloat3(&outP, p);
                return outP;
            };

            auto addL = [&](float r1, float u1, float r2, float u2, XMFLOAT4 col) {
                retVerts.push_back({ addWorldPoint(r1, u1), { 0, 0, 1 }, col });
                retVerts.push_back({ addWorldPoint(r2, u2), { 0, 0, 1 }, col });
            };

            if (isCurrentLocked)
            {
                XMFLOAT4 col = lockInfo.isAimed ? lockRed : lockAmber;

                // 1. Center Crosshair (+)
                float cLen = scale * 0.15f;
                addL(-cLen, 0.0f,  cLen, 0.0f, col);
                addL(0.0f, -cLen, 0.0f,  cLen, col);

                // 2. 4-Corner Brackets: [ + ]
                float bw = scale * 0.60f;
                float bh = scale * 0.60f;
                float blen = scale * 0.20f;

                // Top-Left
                addL(-bw, bh, -bw + blen, bh, col);
                addL(-bw, bh, -bw, bh - blen, col);

                // Top-Right
                addL(bw, bh, bw - blen, bh, col);
                addL(bw, bh, bw, bh - blen, col);

                // Bottom-Left
                addL(-bw, -bh, -bw + blen, -bh, col);
                addL(-bw, -bh, -bw, -bh + blen, col);

                // Bottom-Right
                addL(bw, -bh, bw - blen, -bh, col);
                addL(bw, -bh, bw, -bh + blen, col);

                // 3. Aimed Indicator (AC6-style Outer Diamond Frame)
                if (lockInfo.isAimed)
                {
                    float dSize = scale * 0.78f;
                    addL(0.0f, dSize, dSize, 0.0f, lockRed);
                    addL(dSize, 0.0f, 0.0f, -dSize, lockRed);
                    addL(0.0f, -dSize, -dSize, 0.0f, lockRed);
                    addL(-dSize, 0.0f, 0.0f, dSize, lockRed);
                }

                // 4. Enemy AP / HP Bar above the target
                float barY = bh + scale * 0.20f;
                float barW = scale * 0.65f;
                float barH = scale * 0.06f;
                float hpRatio = target.GetHpRatio();
                XMFLOAT4 hpColor = hpRatio > 0.4f ? hpGreen : hpRed;

                // Background border box
                addL(-barW, barY,  barW, barY, hpBg);
                addL( barW, barY,  barW, barY + barH, hpBg);
                addL( barW, barY + barH, -barW, barY + barH, hpBg);
                addL(-barW, barY + barH, -barW, barY, hpBg);

                // Filled health line
                float fillW = -barW + (barW * 2.0f) * std::clamp(hpRatio, 0.0f, 1.0f);
                if (fillW > -barW + 0.01f)
                {
                    addL(-barW, barY + barH * 0.5f, fillW, barY + barH * 0.5f, hpColor);
                }

                // 4.1 Enemy ACS Bar right below HP Bar
                float acsBarY = barY - scale * 0.075f;
                float acsBarH = scale * 0.045f;
                float acsRatio = target.GetAcsRatio();
                static float s_dummyStgBlink = 0.0f;
                s_dummyStgBlink += 0.016f;
                float dStgAlpha = (std::sin(s_dummyStgBlink * 20.0f) > 0.0f) ? 1.0f : 0.35f;
                XMFLOAT4 acsColor = target.IsStaggered() ? XMFLOAT4(1.0f, 0.18f, 0.12f, dStgAlpha) : (acsRatio > 0.70f ? XMFLOAT4(1.0f, 0.50f, 0.15f, 0.95f) : XMFLOAT4(1.0f, 0.85f, 0.25f, 0.90f));

                addL(-barW, acsBarY,  barW, acsBarY, hpBg);
                addL( barW, acsBarY,  barW, acsBarY + acsBarH, hpBg);
                addL( barW, acsBarY + acsBarH, -barW, acsBarY + acsBarH, hpBg);
                addL(-barW, acsBarY + acsBarH, -barW, acsBarY, hpBg);

                float acsFillW = -barW + (barW * 2.0f) * std::clamp(acsRatio, 0.0f, 1.0f);
                if (acsFillW > -barW + 0.01f)
                {
                    addL(-barW, acsBarY + acsBarH * 0.5f, acsFillW, acsBarY + acsBarH * 0.5f, acsColor);
                }

                // 4.2 Staggered banner if target is staggered
                if (target.IsStaggered())
                {
                    std::string stgStr = "! STAGGERED !";
                    float stgCharW = scale * 0.055f;
                    float stgCharH = scale * 0.10f;
                    float stgCharSp = scale * 0.02f;
                    float stgTotalW = static_cast<float>(stgStr.length()) * (stgCharW + stgCharSp);
                    float stgCurR = -stgTotalW * 0.5f;
                    float stgTextY = barY + barH + scale * 0.10f;
                    XMFLOAT4 stgCol = XMFLOAT4(1.0f, 0.18f, 0.12f, dStgAlpha);

                    for (char c : stgStr)
                    {
                        if (c != ' ')
                        {
                            float l = stgCurR - stgCharW * 0.5f, r = stgCurR + stgCharW * 0.5f;
                            float t = stgTextY + stgCharH * 0.5f, m = stgTextY, b = stgTextY - stgCharH * 0.5f;
                            switch (c)
                            {
                            case '!': addL(stgCurR, t, stgCurR, m, stgCol); addL(stgCurR, b + stgCharH * 0.1f, stgCurR, b, stgCol); break;
                            case 'S': addL(r, t, l, t, stgCol); addL(l, t, l, m, stgCol); addL(l, m, r, m, stgCol); addL(r, m, r, b, stgCol); addL(r, b, l, b, stgCol); break;
                            case 'T': addL(l, t, r, t, stgCol); addL(stgCurR, t, stgCurR, b, stgCol); break;
                            case 'A': addL(l, b, l, t, stgCol); addL(l, t, r, t, stgCol); addL(r, t, r, b, stgCol); addL(l, m, r, m, stgCol); break;
                            case 'G': addL(r, t, l, t, stgCol); addL(l, t, l, b, stgCol); addL(l, b, r, b, stgCol); addL(r, b, r, m, stgCol); addL(r, m, stgCurR, m, stgCol); break;
                            case 'E': addL(r, t, l, t, stgCol); addL(l, t, l, b, stgCol); addL(l, b, r, b, stgCol); addL(l, m, r, m, stgCol); break;
                            case 'R': addL(l, b, l, t, stgCol); addL(l, t, r, t, stgCol); addL(r, t, r, m, stgCol); addL(r, m, l, m, stgCol); addL(stgCurR, m, r, b, stgCol); break;
                            case 'D': addL(l, b, l, t, stgCol); addL(l, t, r, t, stgCol); addL(r, t, r, b, stgCol); addL(r, b, l, b, stgCol); break;
                            default: break;
                            }
                        }
                        stgCurR += stgCharW + stgCharSp;
                    }
                }

                // 5. Distance string below target
                int dInt = static_cast<int>(std::round(dist));
                std::string dStr = "[ " + std::to_string(dInt) + "M ]";
                float charW = scale * 0.07f;
                float charH = scale * 0.13f;
                float charSp = scale * 0.03f;
                float totalW = static_cast<float>(dStr.length()) * (charW + charSp);
                float startR = -totalW * 0.5f;
                float textY = -bh - scale * 0.25f;

                float curR = startR;
                for (char c : dStr)
                {
                    if (c != ' ')
                    {
                        float l = curR - charW * 0.5f;
                        float r = curR + charW * 0.5f;
                        float t = textY + charH * 0.5f;
                        float m = textY;
                        float b = textY - charH * 0.5f;

                        switch (c)
                        {
                        case '0':
                            addL(l, t, r, t, distWhite); addL(r, t, r, b, distWhite); addL(r, b, l, b, distWhite); addL(l, b, l, t, distWhite);
                            break;
                        case '1':
                            addL(r, t, r, b, distWhite);
                            break;
                        case '2':
                            addL(l, t, r, t, distWhite); addL(r, t, r, m, distWhite); addL(r, m, l, m, distWhite); addL(l, m, l, b, distWhite); addL(l, b, r, b, distWhite);
                            break;
                        case '3':
                            addL(l, t, r, t, distWhite); addL(r, t, r, b, distWhite); addL(l, b, r, b, distWhite); addL(l, m, r, m, distWhite);
                            break;
                        case '4':
                            addL(l, t, l, m, distWhite); addL(l, m, r, m, distWhite); addL(r, t, r, b, distWhite);
                            break;
                        case '5':
                            addL(r, t, l, t, distWhite); addL(l, t, l, m, distWhite); addL(l, m, r, m, distWhite); addL(r, m, r, b, distWhite); addL(r, b, l, b, distWhite);
                            break;
                        case '6':
                            addL(r, t, l, t, distWhite); addL(l, t, l, b, distWhite); addL(l, b, r, b, distWhite); addL(r, b, r, m, distWhite); addL(r, m, l, m, distWhite);
                            break;
                        case '7':
                            addL(l, t, r, t, distWhite); addL(r, t, r, b, distWhite);
                            break;
                        case '8':
                            addL(l, t, r, t, distWhite); addL(r, t, r, b, distWhite); addL(r, b, l, b, distWhite); addL(l, b, l, t, distWhite); addL(l, m, r, m, distWhite);
                            break;
                        case '9':
                            addL(r, m, l, m, distWhite); addL(l, m, l, t, distWhite); addL(l, t, r, t, distWhite); addL(r, t, r, b, distWhite); addL(r, b, l, b, distWhite);
                            break;
                        case 'M':
                            addL(l, b, l, t, distWhite); addL(l, t, curR, m, distWhite); addL(curR, m, r, t, distWhite); addL(r, t, r, b, distWhite);
                            break;
                        case '[':
                            addL(r, t, l, t, distWhite); addL(l, t, l, b, distWhite); addL(l, b, r, b, distWhite);
                            break;
                        case ']':
                            addL(l, t, r, t, distWhite); addL(r, t, r, b, distWhite); addL(r, b, l, b, distWhite);
                            break;
                        default:
                            break;
                        }
                    }
                    curR += charW + charSp;
                }
            }
            else
            {
                // Unlocked visible enemies: subtle diamond target marker
                float dSize = scale * 0.40f;
                addL(0.0f, dSize, dSize, 0.0f, idleCyan);
                addL(dSize, 0.0f, 0.0f, -dSize, idleCyan);
                addL(0.0f, -dSize, -dSize, 0.0f, idleCyan);
                addL(-dSize, 0.0f, 0.0f, dSize, idleCyan);
            }
        }

        // Draw Holographic Lock-On and AP/HP Bar for Remote Enemy Mechs
        if (remoteMechs)
        {
            for (const auto& rMech : *remoteMechs)
            {
                if (!rMech.IsActive()) continue;

                XMFLOAT3 pos = rMech.GetPosition();
                XMVECTOR targetPos = XMVectorSet(pos.x, pos.y + 0.8f, pos.z, 0.0f);

                XMVECTOR toCam = camPosVec - targetPos;
                float dist = XMVectorGetX(XMVector3Length(toCam));
                if (dist < 0.5f || dist > 500.0f) continue;

                toCam = XMVector3Normalize(toCam);
                XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
                XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(worldUp, toCam));
                XMVECTOR upVec = XMVector3Cross(toCam, rightVec);

                bool isCurrentLocked = (lockInfo.hasTarget && lockInfo.isRemoteMech && lockInfo.remotePlayerId == rMech.GetPlayerId());
                float scale = std::clamp(dist * 0.045f, 1.2f, 6.5f);

                auto addWorldPointR = [&](float r, float u) -> XMFLOAT3 {
                    XMVECTOR p = targetPos + rightVec * r + upVec * u;
                    XMFLOAT3 outP;
                    XMStoreFloat3(&outP, p);
                    return outP;
                };

                auto addLR = [&](float r1, float u1, float r2, float u2, XMFLOAT4 col) {
                    retVerts.push_back({ addWorldPointR(r1, u1), { 0, 0, 1 }, col });
                    retVerts.push_back({ addWorldPointR(r2, u2), { 0, 0, 1 }, col });
                };

                uint8_t pId = rMech.GetPlayerId();
                std::string playerName = "PLAYER " + std::to_string(pId + 1);

                float barY = scale * 0.70f;
                float barW = scale * 0.75f;
                float barH = scale * 0.07f;

                if (rMech.IsDestroyed())
                {
                    // Render "DESTROYED" status marker above destroyed mech
                    std::string destStr = "DESTROYED";
                    float charH = scale * 0.08f;
                    float charW = charH * 0.70f;
                    float charSp = charW * 0.35f;
                    float totalW = destStr.length() * (charW + charSp);
                    float curR = -totalW * 0.5f;
                    float textY = barY + barH * 0.5f;

                    XMFLOAT4 destCol = { 1.0f, 0.30f, 0.20f, 0.90f };
                    for (char c : destStr)
                    {
                        float l = curR - charW * 0.5f, r = curR + charW * 0.5f;
                        float t = textY + charH * 0.5f, m = textY, b = textY - charH * 0.5f;
                        switch (c)
                        {
                        case 'D': addLR(l, b, l, t, destCol); addLR(l, t, r, t, destCol); addLR(r, t, r, b, destCol); addLR(r, b, l, b, destCol); break;
                        case 'E': addLR(r, t, l, t, destCol); addLR(l, t, l, b, destCol); addLR(l, b, r, b, destCol); addLR(l, m, r, m, destCol); break;
                        case 'S': addLR(r, t, l, t, destCol); addLR(l, t, l, m, destCol); addLR(l, m, r, m, destCol); addLR(r, m, r, b, destCol); addLR(r, b, l, b, destCol); break;
                        case 'T': addLR(l, t, r, t, destCol); addLR(curR, t, curR, b, destCol); break;
                        case 'R': addLR(l, b, l, t, destCol); addLR(l, t, r, t, destCol); addLR(r, t, r, m, destCol); addLR(r, m, l, m, destCol); addLR(curR, m, r, b, destCol); break;
                        case 'O': addLR(l, b, l, t, destCol); addLR(l, t, r, t, destCol); addLR(r, t, r, b, destCol); addLR(r, b, l, b, destCol); break;
                        case 'Y': addLR(l, t, curR, m, destCol); addLR(r, t, curR, m, destCol); addLR(curR, m, curR, b, destCol); break;
                        }
                        curR += charW + charSp;
                    }
                    continue;
                }

                // Enemy HP Bar
                float hpRatio = rMech.GetHpRatio();
                XMFLOAT4 hpColor = hpRatio > 0.4f ? hpGreen : hpRed;

                addLR(-barW, barY,  barW, barY, hpBg);
                addLR( barW, barY,  barW, barY + barH, hpBg);
                addLR( barW, barY + barH, -barW, barY + barH, hpBg);
                addLR(-barW, barY + barH, -barW, barY, hpBg);

                float fillW = -barW + (barW * 2.0f) * std::clamp(hpRatio, 0.0f, 1.0f);
                if (fillW > -barW + 0.01f)
                {
                    addLR(-barW, barY + barH * 0.5f, fillW, barY + barH * 0.5f, hpColor);
                }

                // Enemy ACS Bar right below HP Bar
                float acsBarY = barY - scale * 0.075f;
                float acsBarH = scale * 0.045f;
                float acsRatio = rMech.GetAcsRatio();
                static float s_remoteStgBlink = 0.0f;
                s_remoteStgBlink += 0.016f;
                float rStgAlpha = (std::sin(s_remoteStgBlink * 20.0f) > 0.0f) ? 1.0f : 0.35f;
                XMFLOAT4 acsColor = rMech.IsStaggered() ? XMFLOAT4(1.0f, 0.18f, 0.12f, rStgAlpha) : (acsRatio > 0.70f ? XMFLOAT4(1.0f, 0.50f, 0.15f, 0.95f) : XMFLOAT4(1.0f, 0.85f, 0.25f, 0.90f));

                addLR(-barW, acsBarY,  barW, acsBarY, hpBg);
                addLR( barW, acsBarY,  barW, acsBarY + acsBarH, hpBg);
                addLR( barW, acsBarY + acsBarH, -barW, acsBarY + acsBarH, hpBg);
                addLR(-barW, acsBarY + acsBarH, -barW, acsBarY, hpBg);

                float acsFillW = -barW + (barW * 2.0f) * std::clamp(acsRatio, 0.0f, 1.0f);
                if (acsFillW > -barW + 0.01f)
                {
                    addLR(-barW, acsBarY + acsBarH * 0.5f, acsFillW, acsBarY + acsBarH * 0.5f, acsColor);
                }

                // Staggered banner if remote mech is staggered
                if (rMech.IsStaggered())
                {
                    std::string stgStr = "! STAGGERED !";
                    float stgCharW = scale * 0.055f;
                    float stgCharH = scale * 0.10f;
                    float stgCharSp = scale * 0.02f;
                    float stgTotalW = static_cast<float>(stgStr.length()) * (stgCharW + stgCharSp);
                    float stgCurR = -stgTotalW * 0.5f;
                    float stgTextY = barY + barH + scale * (isCurrentLocked ? 0.22f : 0.10f);
                    XMFLOAT4 stgCol = XMFLOAT4(1.0f, 0.18f, 0.12f, rStgAlpha);

                    for (char c : stgStr)
                    {
                        if (c != ' ')
                        {
                            float l = stgCurR - stgCharW * 0.5f, r = stgCurR + stgCharW * 0.5f;
                            float t = stgTextY + stgCharH * 0.5f, m = stgTextY, b = stgTextY - stgCharH * 0.5f;
                            switch (c)
                            {
                            case '!': addLR(stgCurR, t, stgCurR, m, stgCol); addLR(stgCurR, b + stgCharH * 0.1f, stgCurR, b, stgCol); break;
                            case 'S': addLR(r, t, l, t, stgCol); addLR(l, t, l, m, stgCol); addLR(l, m, r, m, stgCol); addLR(r, m, r, b, stgCol); addLR(r, b, l, b, stgCol); break;
                            case 'T': addLR(l, t, r, t, stgCol); addLR(stgCurR, t, stgCurR, b, stgCol); break;
                            case 'A': addLR(l, b, l, t, stgCol); addLR(l, t, r, t, stgCol); addLR(r, t, r, b, stgCol); addLR(l, m, r, m, stgCol); break;
                            case 'G': addLR(r, t, l, t, stgCol); addLR(l, t, l, b, stgCol); addLR(l, b, r, b, stgCol); addLR(r, b, r, m, stgCol); addLR(r, m, stgCurR, m, stgCol); break;
                            case 'E': addLR(r, t, l, t, stgCol); addLR(l, t, l, b, stgCol); addLR(l, b, r, b, stgCol); addLR(l, m, r, m, stgCol); break;
                            case 'R': addLR(l, b, l, t, stgCol); addLR(l, t, r, t, stgCol); addLR(r, t, r, m, stgCol); addLR(r, m, l, m, stgCol); addLR(stgCurR, m, r, b, stgCol); break;
                            case 'D': addLR(l, b, l, t, stgCol); addLR(l, t, r, t, stgCol); addLR(r, t, r, b, stgCol); addLR(r, b, l, b, stgCol); break;
                            default: break;
                            }
                        }
                        stgCurR += stgCharW + stgCharSp;
                    }
                }

                if (isCurrentLocked)
                {
                    XMFLOAT4 col = lockInfo.isAimed ? lockRed : lockAmber;

                    // Center crosshair
                    float cLen = scale * 0.18f;
                    addLR(-cLen, 0.0f,  cLen, 0.0f, col);
                    addLR(0.0f, -cLen, 0.0f,  cLen, col);

                    // 4-Corner Brackets
                    float bw = scale * 0.65f;
                    float bh = scale * 0.65f;
                    float blen = scale * 0.22f;

                    addLR(-bw, bh, -bw + blen, bh, col);
                    addLR(-bw, bh, -bw, bh - blen, col);
                    addLR(bw, bh, bw - blen, bh, col);
                    addLR(bw, bh, bw, bh - blen, col);
                    addLR(-bw, -bh, -bw + blen, -bh, col);
                    addLR(-bw, -bh, -bw, -bh + blen, col);
                    addLR(bw, -bh, bw - blen, -bh, col);
                    addLR(bw, -bh, bw, -bh + blen, col);

                    // Player Name label above HP bar
                    float pCharW = scale * 0.05f;
                    float pCharH = scale * 0.09f;
                    float pCharSp = scale * 0.02f;
                    float pTotalW = static_cast<float>(playerName.length()) * (pCharW + pCharSp);
                    float pStartR = -pTotalW * 0.5f;
                    float pTextY = barY + barH + scale * 0.08f;
                    float pCurR = pStartR;
                    for (char c : playerName)
                    {
                        if (c != ' ')
                        {
                            float l = pCurR - pCharW * 0.5f, r = pCurR + pCharW * 0.5f;
                            float t = pTextY + pCharH * 0.5f, m = pTextY, b = pTextY - pCharH * 0.5f;
                            switch (c)
                            {
                            case 'P': addLR(l, b, l, t, col); addLR(l, t, r, t, col); addLR(r, t, r, m, col); addLR(r, m, l, m, col); break;
                            case 'L': addLR(l, t, l, b, col); addLR(l, b, r, b, col); break;
                            case 'A': addLR(l, b, l, t, col); addLR(l, t, r, t, col); addLR(r, t, r, b, col); addLR(l, m, r, m, col); break;
                            case 'Y': addLR(l, t, pCurR, m, col); addLR(r, t, pCurR, m, col); addLR(pCurR, m, pCurR, b, col); break;
                            case 'E': addLR(r, t, l, t, col); addLR(l, t, l, b, col); addLR(l, b, r, b, col); addLR(l, m, r, m, col); break;
                            case 'R': addLR(l, b, l, t, col); addLR(l, t, r, t, col); addLR(r, t, r, m, col); addLR(r, m, l, m, col); addLR(pCurR, m, r, b, col); break;
                            case '1': addLR(r, t, r, b, col); break;
                            case '2': addLR(l, t, r, t, col); addLR(r, t, r, m, col); addLR(r, m, l, m, col); addLR(l, m, l, b, col); addLR(l, b, r, b, col); break;
                            case '3': addLR(l, t, r, t, col); addLR(r, t, r, b, col); addLR(l, b, r, b, col); addLR(l, m, r, m, col); break;
                            case '4': addLR(l, t, l, m, col); addLR(l, m, r, m, col); addLR(r, t, r, b, col); break;
                            default: break;
                            }
                        }
                        pCurR += pCharW + pCharSp;
                    }

                    if (lockInfo.isAimed)
                    {
                        float dSize = scale * 0.85f;
                        addLR(0.0f, dSize, dSize, 0.0f, lockRed);
                        addLR(dSize, 0.0f, 0.0f, -dSize, lockRed);
                        addLR(0.0f, -dSize, -dSize, 0.0f, lockRed);
                        addLR(-dSize, 0.0f, 0.0f, dSize, lockRed);
                    }

                    // Distance display
                    int dInt = static_cast<int>(std::round(dist));
                    std::string dStr = "[ " + std::to_string(dInt) + "M ]";
                    float charW = scale * 0.07f;
                    float charH = scale * 0.13f;
                    float charSp = scale * 0.03f;
                    float totalW = static_cast<float>(dStr.length()) * (charW + charSp);
                    float curR = -totalW * 0.5f;
                    float textY = -bh - scale * 0.25f;

                    for (char c : dStr)
                    {
                        if (c != ' ')
                        {
                            float l = curR - charW * 0.5f;
                            float r = curR + charW * 0.5f;
                            float t = textY + charH * 0.5f;
                            float m = textY;
                            float b = textY - charH * 0.5f;

                            switch (c)
                            {
                            case '0': addLR(l, t, r, t, distWhite); addLR(r, t, r, b, distWhite); addLR(r, b, l, b, distWhite); addLR(l, b, l, t, distWhite); break;
                            case '1': addLR(r, t, r, b, distWhite); break;
                            case '2': addLR(l, t, r, t, distWhite); addLR(r, t, r, m, distWhite); addLR(r, m, l, m, distWhite); addLR(l, m, l, b, distWhite); addLR(l, b, r, b, distWhite); break;
                            case '3': addLR(l, t, r, t, distWhite); addLR(r, t, r, b, distWhite); addLR(l, b, r, b, distWhite); addLR(l, m, r, m, distWhite); break;
                            case '4': addLR(l, t, l, m, distWhite); addLR(l, m, r, m, distWhite); addLR(r, t, r, b, distWhite); break;
                            case '5': addLR(r, t, l, t, distWhite); addLR(l, t, l, m, distWhite); addLR(l, m, r, m, distWhite); addLR(r, m, r, b, distWhite); break;
                            case '6': addLR(r, t, l, t, distWhite); addLR(l, t, l, b, distWhite); addLR(l, b, r, b, distWhite); addLR(r, b, r, m, distWhite); addLR(r, m, l, m, distWhite); break;
                            case '7': addLR(l, t, r, t, distWhite); addLR(r, t, r, b, distWhite); break;
                            case '8': addLR(l, t, r, t, distWhite); addLR(r, t, r, b, distWhite); addLR(r, b, l, b, distWhite); addLR(l, b, l, t, distWhite); addLR(l, m, r, m, distWhite); break;
                            case '9': addLR(r, m, l, m, distWhite); addLR(l, m, l, t, distWhite); addLR(l, t, r, t, distWhite); addLR(r, t, r, b, distWhite); break;
                            case 'M': addLR(l, b, l, t, distWhite); addLR(l, t, curR, m, distWhite); addLR(curR, m, r, t, distWhite); addLR(r, t, r, b, distWhite); break;
                            case '[': addLR(r, t, l, t, distWhite); addLR(l, t, l, b, distWhite); addLR(l, b, r, b, distWhite); break;
                            case ']': addLR(l, t, r, t, distWhite); addLR(r, t, r, b, distWhite); addLR(r, b, l, b, distWhite); break;
                            default: break;
                            }
                        }
                        curR += charW + charSp;
                    }
                }
                else
                {
                    // Unlocked enemy mech marker
                    float dSize = scale * 0.45f;
                    addLR(0.0f, dSize, dSize, 0.0f, lockAmber);
                    addLR(dSize, 0.0f, 0.0f, -dSize, lockAmber);
                    addLR(0.0f, -dSize, -dSize, 0.0f, lockAmber);
                    addLR(-dSize, 0.0f, 0.0f, dSize, lockAmber);
                }
            }
        }

        // Draw Holographic Lock-On and AP/HP/ACS Bar for Autonomous AI Combat Bots
        if (aiBots)
        {
            for (const auto& bot : *aiBots)
            {
                if (!bot.IsActive()) continue;

                XMFLOAT3 pos = bot.GetPosition();
                XMVECTOR targetPos = XMVectorSet(pos.x, pos.y + 0.8f, pos.z, 0.0f);

                XMVECTOR toCam = camPosVec - targetPos;
                float dist = XMVectorGetX(XMVector3Length(toCam));
                if (dist < 0.5f || dist > 500.0f) continue;

                toCam = XMVector3Normalize(toCam);
                XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
                XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(worldUp, toCam));
                XMVECTOR upVec = XMVector3Cross(toCam, rightVec);

                bool isCurrentLocked = (lockInfo.hasTarget && lockInfo.isAIBot && lockInfo.botId == bot.GetBotId());
                float scale = std::clamp(dist * 0.045f, 1.2f, 6.5f);

                auto addWorldPointB = [&](float r, float u) -> XMFLOAT3 {
                    XMVECTOR p = targetPos + rightVec * r + upVec * u;
                    XMFLOAT3 outP;
                    XMStoreFloat3(&outP, p);
                    return outP;
                };

                auto addLB = [&](float r1, float u1, float r2, float u2, XMFLOAT4 col) {
                    retVerts.push_back({ addWorldPointB(r1, u1), { 0, 0, 1 }, col });
                    retVerts.push_back({ addWorldPointB(r2, u2), { 0, 0, 1 }, col });
                };

                uint8_t bId = bot.GetBotId();
                std::string botName = "BOT " + std::to_string(bId + 1);

                float barY = scale * 0.70f;
                float barW = scale * 0.75f;
                float barH = scale * 0.07f;

                if (bot.IsDestroyed())
                {
                    std::string destStr = "DESTROYED";
                    float charH = scale * 0.08f;
                    float charW = charH * 0.70f;
                    float charSp = charW * 0.35f;
                    float totalW = destStr.length() * (charW + charSp);
                    float curR = -totalW * 0.5f;
                    float textY = barY + barH * 0.5f;

                    XMFLOAT4 destCol = { 1.0f, 0.30f, 0.20f, 0.90f };
                    for (char c : destStr)
                    {
                        float l = curR - charW * 0.5f, r = curR + charW * 0.5f;
                        float t = textY + charH * 0.5f, m = textY, b = textY - charH * 0.5f;
                        switch (c)
                        {
                        case 'D': addLB(l, b, l, t, destCol); addLB(l, t, r, t, destCol); addLB(r, t, r, b, destCol); addLB(r, b, l, b, destCol); break;
                        case 'E': addLB(r, t, l, t, destCol); addLB(l, t, l, b, destCol); addLB(l, b, r, b, destCol); addLB(l, m, r, m, destCol); break;
                        case 'S': addLB(r, t, l, t, destCol); addLB(l, t, l, m, destCol); addLB(l, m, r, m, destCol); addLB(r, m, r, b, destCol); addLB(r, b, l, b, destCol); break;
                        case 'T': addLB(l, t, r, t, destCol); addLB(curR, t, curR, b, destCol); break;
                        case 'R': addLB(l, b, l, t, destCol); addLB(l, t, r, t, destCol); addLB(r, t, r, m, destCol); addLB(r, m, l, m, destCol); addLB(curR, m, r, b, destCol); break;
                        case 'O': addLB(l, b, l, t, destCol); addLB(l, t, r, t, destCol); addLB(r, t, r, b, destCol); addLB(r, b, l, b, destCol); break;
                        case 'Y': addLB(l, t, curR, m, destCol); addLB(r, t, curR, m, destCol); addLB(curR, m, curR, b, destCol); break;
                        }
                        curR += charW + charSp;
                    }
                    continue;
                }

                // Bot HP Bar
                float hpRatio = bot.GetHpRatio();
                XMFLOAT4 hpColor = hpRatio > 0.4f ? hpGreen : hpRed;

                addLB(-barW, barY,  barW, barY, hpBg);
                addLB( barW, barY,  barW, barY + barH, hpBg);
                addLB( barW, barY + barH, -barW, barY + barH, hpBg);
                addLB(-barW, barY + barH, -barW, barY, hpBg);

                float fillW = -barW + (barW * 2.0f) * std::clamp(hpRatio, 0.0f, 1.0f);
                if (fillW > -barW + 0.01f)
                {
                    addLB(-barW, barY + barH * 0.5f, fillW, barY + barH * 0.5f, hpColor);
                }

                // Bot ACS Bar
                float acsBarY = barY - scale * 0.075f;
                float acsBarH = scale * 0.045f;
                float acsRatio = bot.GetAcsRatio();
                static float s_botStgBlink = 0.0f;
                s_botStgBlink += 0.016f;
                float bStgAlpha = (std::sin(s_botStgBlink * 20.0f) > 0.0f) ? 1.0f : 0.35f;
                XMFLOAT4 acsColor = bot.IsStaggered() ? XMFLOAT4(1.0f, 0.18f, 0.12f, bStgAlpha) : (acsRatio > 0.70f ? XMFLOAT4(1.0f, 0.50f, 0.15f, 0.95f) : XMFLOAT4(1.0f, 0.85f, 0.25f, 0.90f));

                addLB(-barW, acsBarY,  barW, acsBarY, hpBg);
                addLB( barW, acsBarY,  barW, acsBarY + acsBarH, hpBg);
                addLB( barW, acsBarY + acsBarH, -barW, acsBarY + acsBarH, hpBg);
                addLB(-barW, acsBarY + acsBarH, -barW, acsBarY, hpBg);

                float acsFillW = -barW + (barW * 2.0f) * std::clamp(acsRatio, 0.0f, 1.0f);
                if (acsFillW > -barW + 0.01f)
                {
                    addLB(-barW, acsBarY + acsBarH * 0.5f, acsFillW, acsBarY + acsBarH * 0.5f, acsColor);
                }

                if (bot.IsStaggered())
                {
                    std::string stgStr = "! STAGGERED !";
                    float stgCharW = scale * 0.055f;
                    float stgCharH = scale * 0.10f;
                    float stgCharSp = scale * 0.02f;
                    float stgTotalW = static_cast<float>(stgStr.length()) * (stgCharW + stgCharSp);
                    float stgCurR = -stgTotalW * 0.5f;
                    float stgTextY = barY + barH + scale * (isCurrentLocked ? 0.22f : 0.10f);
                    XMFLOAT4 stgCol = XMFLOAT4(1.0f, 0.18f, 0.12f, bStgAlpha);

                    for (char c : stgStr)
                    {
                        if (c != ' ')
                        {
                            float l = stgCurR - stgCharW * 0.5f, r = stgCurR + stgCharW * 0.5f;
                            float t = stgTextY + stgCharH * 0.5f, m = stgTextY, b = stgTextY - stgCharH * 0.5f;
                            switch (c)
                            {
                            case '!': addLB(stgCurR, t, stgCurR, m, stgCol); addLB(stgCurR, b + stgCharH * 0.1f, stgCurR, b, stgCol); break;
                            case 'S': addLB(r, t, l, t, stgCol); addLB(l, t, l, m, stgCol); addLB(l, m, r, m, stgCol); addLB(r, m, r, b, stgCol); addLB(r, b, l, b, stgCol); break;
                            case 'T': addLB(l, t, r, t, stgCol); addLB(stgCurR, t, stgCurR, b, stgCol); break;
                            case 'A': addLB(l, b, l, t, stgCol); addLB(l, t, r, t, stgCol); addLB(r, t, r, b, stgCol); addLB(l, m, r, m, stgCol); break;
                            case 'G': addLB(r, t, l, t, stgCol); addLB(l, t, l, b, stgCol); addLB(l, b, r, b, stgCol); addLB(r, b, r, m, stgCol); addLB(r, m, stgCurR, m, stgCol); break;
                            case 'E': addLB(r, t, l, t, stgCol); addLB(l, t, l, b, stgCol); addLB(l, b, r, b, stgCol); addLB(l, m, r, m, stgCol); break;
                            case 'R': addLB(l, b, l, t, stgCol); addLB(l, t, r, t, stgCol); addLB(r, t, r, m, stgCol); addLB(r, m, l, m, stgCol); addLB(stgCurR, m, r, b, stgCol); break;
                            case 'D': addLB(l, b, l, t, stgCol); addLB(l, t, r, t, stgCol); addLB(r, t, r, b, stgCol); addLB(r, b, l, b, stgCol); break;
                            default: break;
                            }
                        }
                        stgCurR += stgCharW + stgCharSp;
                    }
                }

                if (isCurrentLocked)
                {
                    XMFLOAT4 col = lockInfo.isAimed ? lockRed : lockAmber;

                    // Center crosshair
                    float cLen = scale * 0.18f;
                    addLB(-cLen, 0.0f,  cLen, 0.0f, col);
                    addLB(0.0f, -cLen, 0.0f,  cLen, col);

                    // 4-Corner Brackets
                    float bw = scale * 0.65f;
                    float bh = scale * 0.65f;
                    float blen = scale * 0.22f;

                    addLB(-bw, bh, -bw + blen, bh, col);
                    addLB(-bw, bh, -bw, bh - blen, col);
                    addLB(bw, bh, bw - blen, bh, col);
                    addLB(bw, bh, bw, bh - blen, col);
                    addLB(-bw, -bh, -bw + blen, -bh, col);
                    addLB(-bw, -bh, -bw, -bh + blen, col);
                    addLB(bw, -bh, bw - blen, -bh, col);
                    addLB(bw, -bh, bw, -bh + blen, col);

                    // Bot Name label above HP bar
                    float pCharW = scale * 0.05f;
                    float pCharH = scale * 0.09f;
                    float pCharSp = scale * 0.02f;
                    float pTotalW = static_cast<float>(botName.length()) * (pCharW + pCharSp);
                    float pStartR = -pTotalW * 0.5f;
                    float pTextY = barY + barH + scale * 0.08f;
                    float pCurR = pStartR;
                    for (char c : botName)
                    {
                        if (c != ' ')
                        {
                            float l = pCurR - pCharW * 0.5f, r = pCurR + pCharW * 0.5f;
                            float t = pTextY + pCharH * 0.5f, m = pTextY, b = pTextY - pCharH * 0.5f;
                            switch (c)
                            {
                            case 'B': addLB(l, b, l, t, col); addLB(l, t, r, t, col); addLB(r, t, r, m, col); addLB(r, m, l, m, col); addLB(r, m, r, b, col); addLB(r, b, l, b, col); break;
                            case 'O': addLB(l, b, l, t, col); addLB(l, t, r, t, col); addLB(r, t, r, b, col); addLB(r, b, l, b, col); break;
                            case 'T': addLB(l, t, r, t, col); addLB(pCurR, t, pCurR, b, col); break;
                            case '1': addLB(r, t, r, b, col); break;
                            case '2': addLB(l, t, r, t, col); addLB(r, t, r, m, col); addLB(r, m, l, m, col); addLB(l, m, l, b, col); addLB(l, b, r, b, col); break;
                            case '3': addLB(l, t, r, t, col); addLB(r, t, r, b, col); addLB(l, b, r, b, col); addLB(l, m, r, m, col); break;
                            case '4': addLB(l, t, l, m, col); addLB(l, m, r, m, col); addLB(r, t, r, b, col); break;
                            default: break;
                            }
                        }
                        pCurR += pCharW + pCharSp;
                    }

                    if (lockInfo.isAimed)
                    {
                        float dSize = scale * 0.85f;
                        addLB(0.0f, dSize, dSize, 0.0f, lockRed);
                        addLB(dSize, 0.0f, 0.0f, -dSize, lockRed);
                        addLB(0.0f, -dSize, -dSize, 0.0f, lockRed);
                        addLB(-dSize, 0.0f, 0.0f, dSize, lockRed);
                    }

                    // Distance display
                    int dInt = static_cast<int>(std::round(dist));
                    std::string dStr = "[ " + std::to_string(dInt) + "M ]";
                    float charW = scale * 0.07f;
                    float charH = scale * 0.13f;
                    float charSp = scale * 0.03f;
                    float totalW = static_cast<float>(dStr.length()) * (charW + charSp);
                    float curR = -totalW * 0.5f;
                    float textY = -bh - scale * 0.25f;

                    for (char c : dStr)
                    {
                        if (c != ' ')
                        {
                            float l = curR - charW * 0.5f;
                            float r = curR + charW * 0.5f;
                            float t = textY + charH * 0.5f;
                            float m = textY;
                            float b = textY - charH * 0.5f;

                            switch (c)
                            {
                            case '0': addLB(l, t, r, t, distWhite); addLB(r, t, r, b, distWhite); addLB(r, b, l, b, distWhite); addLB(l, b, l, t, distWhite); break;
                            case '1': addLB(r, t, r, b, distWhite); break;
                            case '2': addLB(l, t, r, t, distWhite); addLB(r, t, r, m, distWhite); addLB(r, m, l, m, distWhite); addLB(l, m, l, b, distWhite); addLB(l, b, r, b, distWhite); break;
                            case '3': addLB(l, t, r, t, distWhite); addLB(r, t, r, b, distWhite); addLB(l, b, r, b, distWhite); addLB(l, m, r, m, distWhite); break;
                            case '4': addLB(l, t, l, m, distWhite); addLB(l, m, r, m, distWhite); addLB(r, t, r, b, distWhite); break;
                            case '5': addLB(r, t, l, t, distWhite); addLB(l, t, l, m, distWhite); addLB(l, m, r, m, distWhite); addLB(r, m, r, b, distWhite); break;
                            case '6': addLB(r, t, l, t, distWhite); addLB(l, t, l, b, distWhite); addLB(l, b, r, b, distWhite); addLB(r, b, r, m, distWhite); addLB(r, m, l, m, distWhite); break;
                            case '7': addLB(l, t, r, t, distWhite); addLB(r, t, r, b, distWhite); break;
                            case '8': addLB(l, t, r, t, distWhite); addLB(r, t, r, b, distWhite); addLB(r, b, l, b, distWhite); addLB(l, b, l, t, distWhite); addLB(l, m, r, m, distWhite); break;
                            case '9': addLB(r, m, l, m, distWhite); addLB(l, m, l, t, distWhite); addLB(l, t, r, t, distWhite); addLB(r, t, r, b, distWhite); break;
                            case 'M': addLB(l, b, l, t, distWhite); addLB(l, t, curR, m, distWhite); addLB(curR, m, r, t, distWhite); addLB(r, t, r, b, distWhite); break;
                            case '[': addLB(r, t, l, t, distWhite); addLB(l, t, l, b, distWhite); addLB(l, b, r, b, distWhite); break;
                            case ']': addLB(l, t, r, t, distWhite); addLB(r, t, r, b, distWhite); addLB(r, b, l, b, distWhite); break;
                            default: break;
                            }
                        }
                        curR += charW + charSp;
                    }
                }
                else
                {
                    // Unlocked enemy bot marker
                    float dSize = scale * 0.45f;
                    addLB(0.0f, dSize, dSize, 0.0f, lockAmber);
                    addLB(dSize, 0.0f, 0.0f, -dSize, lockAmber);
                    addLB(0.0f, -dSize, -dSize, 0.0f, lockAmber);
                    addLB(-dSize, 0.0f, 0.0f, dSize, lockAmber);
                }
            }
        }

        if (!retVerts.empty())
        {
            D3D11_MAPPED_SUBRESOURCE dynMap;
            if (SUCCEEDED(m_context->Map(m_dynamicReticleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dynMap)))
            {
                UINT copyCount = static_cast<UINT>(std::min(retVerts.size(), size_t(2048)));
                memcpy(dynMap.pData, retVerts.data(), sizeof(Vertex) * copyCount);
                m_context->Unmap(m_dynamicReticleBuffer.Get(), 0);

                UINT stride = sizeof(Vertex);
                UINT offset = 0;
                m_context->IASetVertexBuffers(0, 1, m_dynamicReticleBuffer.GetAddressOf(), &stride, &offset);
                m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                m_context->Draw(copyCount, 0);
            }
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
            if (p.weaponType == WeaponType::HeavyBazooka)
            {
                // Thicker fiery blast head for Heavy Bazooka
                pVerts.push_back({ p.prevPosition, { 0, 1, 0 }, p.color });
                pVerts.push_back({ p.position,     { 0, 1, 0 }, p.color });

                XMFLOAT4 coreCol = { 1.0f, 0.95f, 0.6f, 1.0f };
                pVerts.push_back({ { p.position.x - 0.25f, p.position.y, p.position.z }, { 0, 1, 0 }, coreCol });
                pVerts.push_back({ { p.position.x + 0.25f, p.position.y, p.position.z }, { 0, 1, 0 }, coreCol });
                pVerts.push_back({ { p.position.x, p.position.y - 0.25f, p.position.z }, { 0, 1, 0 }, coreCol });
                pVerts.push_back({ { p.position.x, p.position.y + 0.25f, p.position.z }, { 0, 1, 0 }, coreCol });
            }
            else if (p.weaponType == WeaponType::MissilePod)
            {
                // Micro-missile smoke & flame trail
                XMFLOAT4 smokeCol = { 0.95f, 0.95f, 0.95f, 0.75f };
                XMFLOAT4 fireCol  = { 1.0f, 0.65f, 0.15f, 1.0f };
                pVerts.push_back({ p.prevPosition, { 0, 1, 0 }, smokeCol });
                pVerts.push_back({ p.position,     { 0, 1, 0 }, fireCol });
            }
            else
            {
                pVerts.push_back({ p.prevPosition, { 0, 1, 0 }, p.color });
                pVerts.push_back({ p.position,     { 0, 1, 0 }, p.color });
            }
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

    void D3D11Renderer::RenderMechWeapon(const WeaponInstance& weapon, const XMMATRIX& mountWorld, const XMMATRIX& view, const XMMATRIX& proj)
    {
        if (!weapon.IsValid()) return;

        // 1. Try loading custom 3D model (.glb) from assets/models/weapons/
        std::string glbPath = "assets/models/weapons/" + weapon.data.id + ".glb";
        auto model = m_modelManager.GetOrLoad(m_device.Get(), glbPath);
        if (!model)
        {
            // Also try with short name e.g. "assets/models/weapons/RA-RIFLE.glb"
            std::string altPath = "assets/models/weapons/" + weapon.data.shortName + ".glb";
            model = m_modelManager.GetOrLoad(m_device.Get(), altPath);
        }

        XMMATRIX spinMat = (weapon.data.type == WeaponType::GatlingGun)
            ? XMMatrixRotationZ(weapon.spinAngle)
            : XMMatrixIdentity();

        if (model && model->IsValid())
        {
            const auto& mSize = model->GetSize();
            const auto& mCenter = model->GetCenter();

            float scaleX = weapon.data.modelSize.x / std::max(0.01f, mSize.x);
            float scaleY = weapon.data.modelSize.y / std::max(0.01f, mSize.y);
            float scaleZ = weapon.data.modelSize.z / std::max(0.01f, mSize.z);

            // Center origin, scale, spin barrel for rotary weapons, and fit mech mount specifications
            XMMATRIX modelTrans = XMMatrixTranslation(-mCenter.x, -mCenter.y, -mCenter.z)
                                * XMMatrixScaling(scaleX, scaleY, scaleZ)
                                * spinMat
                                * mountWorld;

            D3D11_MAPPED_SUBRESOURCE mapped;
            if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
                cb->world = modelTrans;
                cb->view = view;
                cb->projection = proj;
                cb->customParams = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f); // Lit Mech Shader
                m_context->Unmap(m_constantBuffer.Get(), 0);
            }

            model->Render(m_context.Get(), m_constantBuffer.Get(), modelTrans);
            return;
        }

        // 2. Procedural mesh fallback
        auto it = m_weaponMeshes.find(weapon.data.type);
        if (it == m_weaponMeshes.end()) return;

        const auto& mesh = it->second;
        if (!mesh.vertexBuffer || !mesh.indexBuffer || mesh.indexCount == 0) return;

        XMMATRIX procWorld = spinMat * mountWorld;

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = procWorld;
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f); // Lit Mech Shader
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_context->DrawIndexed(mesh.indexCount, 0, 0);
    }

    void D3D11Renderer::RenderFramePart(const FrameInstance& part, FrameSlot slot, const XMMATRIX& mountWorld, const XMMATRIX& view, const XMMATRIX& proj, bool isFPV)
    {
        if (!part.IsValid()) return;

        // 1. Try glTF custom 3D model (.glb) from assets/models/frame/<slot>/<id>.glb
        std::string slotDir = "head";
        if (slot == FrameSlot::Core) slotDir = "core";
        else if (slot == FrameSlot::Arms) slotDir = "arms";
        else if (slot == FrameSlot::Legs) slotDir = "legs";

        std::string glbPath = "assets/models/frame/" + slotDir + "/" + part.data.id + ".glb";
        auto model = m_modelManager.GetOrLoad(m_device.Get(), glbPath);
        if (!model)
        {
            std::string altPath = "assets/models/frame/" + slotDir + "/" + part.data.shortName + ".glb";
            model = m_modelManager.GetOrLoad(m_device.Get(), altPath);
        }

        if (model && model->IsValid())
        {
            const auto& mCenter = model->GetCenter();
            XMMATRIX modelTrans = XMMatrixTranslation(-mCenter.x, -mCenter.y, -mCenter.z) * mountWorld;

            D3D11_MAPPED_SUBRESOURCE mapped;
            if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
                cb->world = modelTrans;
                cb->view = view;
                cb->projection = proj;
                cb->customParams = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f); // Lit Mech Shader
                m_context->Unmap(m_constantBuffer.Get(), 0);
            }

            model->Render(m_context.Get(), m_constantBuffer.Get(), modelTrans);
            return;
        }

        // 2. Procedural frame mesh fallback
        auto it = m_frameMeshes.find(part.data.type);
        if (it == m_frameMeshes.end()) return;

        const auto& mesh = it->second;
        if (!mesh.vertexBuffer || !mesh.indexBuffer || mesh.indexCount == 0) return;

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = mountWorld;
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f); // Lit Mech Shader
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_context->DrawIndexed(mesh.indexCount, 0, 0);
    }

    void D3D11Renderer::RenderMech(const MechController& mech, const WeaponSystem& weapons, const FrameSystem* frames, const XMMATRIX& view, const XMMATRIX& proj, bool isFPV)
    {
        XMFLOAT3 pos = mech.GetPosition();
        float hoverBob = mech.GetHoverBobOffset();

        float abPitch = mech.IsBoostKicking() ? 0.42f : (mech.IsAssaultBoost() ? 0.35f : 0.0f);
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(mech.GetPitch() * 0.3f + abPitch, mech.GetYaw(), mech.GetRoll());
        XMMATRIX trans = XMMatrixTranslation(pos.x, pos.y + hoverBob, pos.z);
        XMMATRIX world = rot * trans;

        if (frames)
        {
            // 1. Legs (Rendered in TPS, skipped or lower in FPV)
            if (!isFPV)
            {
                XMMATRIX legsWorld = world;
                if (mech.IsBoostKicking())
                {
                    // Dynamic procedural kick pose: extend legs forward and up with high-energy thrust angle
                    float kickProgress = 1.0f - (mech.GetBoostKickTimer() / (mech.GetBoostKickDuration() > 0.001f ? mech.GetBoostKickDuration() : 0.42f));
                    float ext = std::sin(std::clamp(kickProgress, 0.0f, 1.0f) * XM_PI);
                    XMMATRIX kickOffset = XMMatrixRotationX(-XM_PIDIV4 * 0.45f * ext) * XMMatrixTranslation(0.0f, 0.28f * ext, 0.85f * ext);
                    legsWorld = kickOffset * world;
                }
                RenderFramePart(frames->GetSlot(FrameSlot::Legs), FrameSlot::Legs, legsWorld, view, proj, isFPV);
            }

            // 2. Core (Rendered in TPS, skipped in FPV to not block cockpit)
            if (!isFPV)
            {
                RenderFramePart(frames->GetSlot(FrameSlot::Core), FrameSlot::Core, world, view, proj, isFPV);
            }

            // 3. Head (Rendered in TPS, skipped in FPV)
            if (!isFPV)
            {
                RenderFramePart(frames->GetSlot(FrameSlot::Head), FrameSlot::Head, world, view, proj, isFPV);
            }

            // 4. Arms (Rendered in both TPS and FPV cockpit view!)
            RenderFramePart(frames->GetSlot(FrameSlot::Arms), FrameSlot::Arms, world, view, proj, isFPV);
        }
        else
        {
            // Legacy single-mesh fallback
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

        // Render modular weapons for each slot with procedural recoil and aim elevation
        float aimPitch = mech.GetAimPitch();
        float rRecoil = mech.GetRecoilRight();
        float lRecoil = mech.GetRecoilLeft();

        // 1. Right Arm Weapon
        XMMATRIX raMat = XMMatrixRotationX(-aimPitch) * XMMatrixTranslation(0.95f, 1.0f, 0.40f - rRecoil) * world;
        RenderMechWeapon(weapons.GetSlot(WeaponSlot::RightArm), raMat, view, proj);

        // 2. Left Arm Weapon
        XMMATRIX laMat = XMMatrixRotationX(-aimPitch) * XMMatrixTranslation(-0.95f, 1.0f, 0.40f - lRecoil) * world;
        RenderMechWeapon(weapons.GetSlot(WeaponSlot::LeftArm), laMat, view, proj);

        // 3. Right Back Hanger Weapon (mounted tilted upward/backward on shoulder)
        XMMATRIX rbMat = XMMatrixRotationRollPitchYaw(1.22f, 0.0f, 0.0f) * XMMatrixTranslation(0.80f, 2.15f, -0.40f) * world;
        RenderMechWeapon(weapons.GetSlot(WeaponSlot::RightBack), rbMat, view, proj);

        // 4. Left Back Hanger Weapon (mounted tilted upward/backward on shoulder)
        XMMATRIX lbMat = XMMatrixRotationRollPitchYaw(1.22f, 0.0f, 0.0f) * XMMatrixTranslation(-0.80f, 2.15f, -0.40f) * world;
        RenderMechWeapon(weapons.GetSlot(WeaponSlot::LeftBack), lbMat, view, proj);

        // 5. Thruster Jet Plumes (Back Main, Shoulder Boosters, Foot Verniers)
        RenderMechThrusters(mech, world, view, proj, isFPV);

        // 6. Laser Blade Slash Plasma Arc
        if (weapons.IsBladeSlashing())
        {
            float slashProgress = weapons.GetBladeSlashTimer() / 0.38f;
            float bladeLen = 5.2f * std::sin(slashProgress * XM_PI);
            float bladeRad = 0.38f;
            XMMATRIX armMount = weapons.IsBladeRightArm() ? raMat : laMat;
            XMMATRIX bladeWorld = XMMatrixRotationY(XM_PI) * XMMatrixTranslation(0.0f, 0.0f, 0.65f) * armMount;

            XMFLOAT4 bladeOuter = { 0.15f, 0.95f, 0.65f, 1.0f }; // Moonlight Green
            XMFLOAT4 bladeCore  = { 0.95f, 1.00f, 0.95f, 1.0f }; // Pure White
            m_context->OMSetBlendState(m_additiveBlendState.Get(), nullptr, 0xFFFFFFFF);
            m_context->OMSetDepthStencilState(m_thrusterDepthState.Get(), 0);
            RenderJetPlume(bladeWorld, bladeLen, bladeRad, bladeOuter, bladeCore, view, proj);
            m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
            m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
        }
    }

    void D3D11Renderer::RenderEnemyMech(const RemoteMech& remoteMech, const XMMATRIX& view, const XMMATRIX& proj)
    {
        if (!remoteMech.IsAlive()) return;

        XMFLOAT3 pos = remoteMech.GetPosition();
        XMFLOAT3 rot = remoteMech.GetRotation(); // Yaw, Pitch, Roll

        float abPitch = remoteMech.IsBoostKicking() ? 0.42f : (remoteMech.IsAssaultBoosting() ? 0.35f : 0.0f);
        XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(rot.y * 0.3f + abPitch, rot.x, rot.z);
        XMMATRIX transMat = XMMatrixTranslation(pos.x, pos.y, pos.z);
        XMMATRIX world = rotMat * transMat;

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = world;
            cb->view = view;
            cb->projection = proj;
            // Hit flashing if hitFlashTimer > 0 (unlit white/emissive)
            cb->customParams = remoteMech.IsHitFlashing() ? XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) : XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        uint8_t pId = remoteMech.GetPlayerId() % 4;
        ID3D11Buffer* vBuf = m_playerMechVertexBuffer[pId].Get() ? m_playerMechVertexBuffer[pId].Get() : m_enemyMechVertexBuffer.Get();
        m_context->IASetVertexBuffers(0, 1, &vBuf, &stride, &offset);
        m_context->IASetIndexBuffer(m_mechIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_context->DrawIndexed(m_mechIndexCount, 0, 0);

        // Render Enemy Mech weapons (Right Rifle, Left Beam, Left Back Plasma Hanger)
        WeaponInstance rWp, lWp, lbWp;
        rWp.data.type = WeaponType::KineticRifle;
        lWp.data.type = WeaponType::BeamRifle;
        lbWp.data.type = WeaponType::HeavyPlasma;

        XMMATRIX raMat = XMMatrixTranslation(0.95f, 1.0f, 0.40f) * world;
        RenderMechWeapon(rWp, raMat, view, proj);

        XMMATRIX laMat = XMMatrixTranslation(-0.95f, 1.0f, 0.40f) * world;
        RenderMechWeapon(lWp, laMat, view, proj);

        XMMATRIX lbMat = XMMatrixRotationRollPitchYaw(1.22f, 0.0f, 0.0f) * XMMatrixTranslation(-0.80f, 2.15f, -0.40f) * world;
        RenderMechWeapon(lbWp, lbMat, view, proj);

        // Thruster Jet Plumes for remote enemy mech
        RenderRemoteMechThrusters(remoteMech, world, view, proj);
    }

    void D3D11Renderer::RenderJetPlume(const XMMATRIX& nozzleWorld, float length, float radius, const XMFLOAT4& outerColor, const XMFLOAT4& coreColor, const XMMATRIX& view, const XMMATRIX& proj)
    {
        if (length <= 0.05f || radius <= 0.01f || !m_jetVertexBuffer || !m_jetIndexBuffer || m_jetIndexCount == 0) return;

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, m_jetVertexBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetIndexBuffer(m_jetIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // Pass 1: Outer glowing plasma plume mantle
        XMMATRIX outerWorld = XMMatrixScaling(radius, radius, length) * nozzleWorld;
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = outerWorld;
            cb->view = view;
            cb->projection = proj;
            // x: 0 (Unlit/Emissive mode in basic.hlsl), yzw: outerColor tint
            cb->customParams = XMFLOAT4(0.0f, outerColor.x, outerColor.y, outerColor.z);
            m_context->Unmap(m_constantBuffer.Get(), 0);
            m_context->DrawIndexed(m_jetIndexCount, 0, 0);
        }

        // Pass 2: Inner superheated core streak (Brighter, thinner, concentrated)
        XMMATRIX coreWorld = XMMatrixScaling(radius * 0.42f, radius * 0.42f, length * 0.72f) * nozzleWorld;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = coreWorld;
            cb->view = view;
            cb->projection = proj;
            // x: 0 (Unlit/Emissive mode in basic.hlsl), yzw: coreColor tint
            cb->customParams = XMFLOAT4(0.0f, coreColor.x, coreColor.y, coreColor.z);
            m_context->Unmap(m_constantBuffer.Get(), 0);
            m_context->DrawIndexed(m_jetIndexCount, 0, 0);
        }
    }

    void D3D11Renderer::RenderMechThrusters(const MechController& mech, const XMMATRIX& mechWorld, const XMMATRIX& view, const XMMATRIX& proj, bool isFPV)
    {
        if (mech.IsDestroyed()) return;

        bool isKick = mech.IsBoostKicking();
        bool isAB = mech.IsAssaultBoost();
        bool isQB = mech.IsQuickBoost();
        bool isBoost = mech.IsBoostMode();
        bool isAscend = mech.IsAscending();

        // If no thrusters active, return early
        if (!isKick && !isAB && !isQB && !isBoost && !isAscend) return;

        static uint32_t s_frameCounter = 0;
        s_frameCounter++;
        float flicker = 1.0f + 0.07f * std::sin(s_frameCounter * 0.45f) + 0.05f * std::cos(s_frameCounter * 0.95f);

        float mainLen = 0.0f;
        float mainRad = 0.0f;
        float shoulderLen = 0.0f;
        float shoulderRad = 0.0f;
        float footLen = 0.0f;
        float footRad = 0.0f;

        XMFLOAT4 outerColor = { 0.20f, 0.85f, 1.0f, 1.0f }; // Cyber Cyan
        XMFLOAT4 coreColor  = { 0.95f, 1.00f, 1.0f, 1.0f }; // Pure White

        if (isKick)
        {
            // Boost Kick: Colossal thrust blast pushing mech forward into impact
            mainLen = (8.5f + 1.5f * std::sin(s_frameCounter * 0.5f)) * flicker;
            mainRad = 0.55f * flicker;
            shoulderLen = 4.2f * flicker;
            shoulderRad = 0.32f * flicker;
            footLen = 3.2f * flicker;
            footRad = 0.28f * flicker;
            outerColor = { 1.0f, 0.25f, 0.05f, 1.0f }; // Ultra-hot Crimson/Orange
            coreColor  = { 1.0f, 1.0f, 0.9f, 1.0f };   // White-hot plasma core
        }
        else if (isAB)
        {
            // Assault Boost: Massive afterburner flames
            mainLen = (6.8f + 1.2f * std::sin(s_frameCounter * 0.35f)) * flicker;
            mainRad = 0.44f * flicker;
            shoulderLen = 3.6f * flicker;
            shoulderRad = 0.26f * flicker;
            footLen = 2.4f * flicker;
            footRad = 0.22f * flicker;
            outerColor = { 1.0f, 0.45f, 0.10f, 1.0f }; // Blazing Orange
            coreColor  = { 1.0f, 0.95f, 0.75f, 1.0f }; // Superheated Yellow-White
        }
        else if (isQB)
        {
            // Quick Boost: Explosive shockwave burst decaying over time
            float qbProgress = mech.GetQbDuration() > 0.001f ? (mech.GetQbTimer() / mech.GetQbDuration()) : 1.0f;
            float burstScale = std::clamp(qbProgress, 0.0f, 1.0f);
            mainLen = (4.6f * burstScale + 1.2f) * flicker;
            mainRad = (0.42f * burstScale + 0.16f) * flicker;
            shoulderLen = (3.4f * burstScale + 0.8f) * flicker;
            shoulderRad = (0.32f * burstScale + 0.12f) * flicker;
            footLen = (2.2f * burstScale + 0.5f) * flicker;
            footRad = (0.24f * burstScale + 0.10f) * flicker;
            outerColor = { 0.25f, 0.92f, 1.0f, 1.0f }; // High-voltage Cyan
            coreColor  = { 1.0f, 1.0f, 1.0f, 1.0f };
        }
        else
        {
            // Standard Boost / Hover / Ascend
            float curSpeed = mech.GetCurrentSpeed();
            float speedFactor = std::clamp(curSpeed / 55.0f, 0.0f, 1.0f);

            if (isBoost)
            {
                mainLen = (1.3f + speedFactor * 1.5f) * flicker;
                mainRad = (0.20f + speedFactor * 0.08f) * flicker;
                shoulderLen = (0.6f + speedFactor * 0.8f) * flicker;
                shoulderRad = (0.12f + speedFactor * 0.05f) * flicker;
                footLen = 0.8f * flicker;
                footRad = 0.14f * flicker;
            }

            if (isAscend)
            {
                footLen = std::max(footLen, (2.8f + 0.4f * std::sin(s_frameCounter * 0.4f)) * flicker);
                footRad = std::max(footRad, 0.28f * flicker);
                mainLen = std::max(mainLen, 1.8f * flicker);
                mainRad = std::max(mainRad, 0.22f * flicker);
            }
        }

        // Set Additive Blending and Depth Test (Read Only)
        m_context->OMSetBlendState(m_additiveBlendState.Get(), nullptr, 0xFFFFFFFF);
        m_context->OMSetDepthStencilState(m_thrusterDepthState.Get(), 0);

        // 1. Back Main Thrusters (Left & Right)
        if (mainLen > 0.05f)
        {
            XMMATRIX leftMainMat  = XMMatrixTranslation(-0.35f, 1.60f, -0.55f) * mechWorld;
            XMMATRIX rightMainMat = XMMatrixTranslation( 0.35f, 1.60f, -0.55f) * mechWorld;
            RenderJetPlume(leftMainMat,  mainLen, mainRad, outerColor, coreColor, view, proj);
            RenderJetPlume(rightMainMat, mainLen, mainRad, outerColor, coreColor, view, proj);
        }

        // 2. Shoulder Side Boosters (Left & Right, slightly canted outward)
        if (shoulderLen > 0.05f)
        {
            XMMATRIX leftShoulderMat  = XMMatrixRotationY( 0.10f) * XMMatrixTranslation(-0.78f, 1.85f, -0.42f) * mechWorld;
            XMMATRIX rightShoulderMat = XMMatrixRotationY(-0.10f) * XMMatrixTranslation( 0.78f, 1.85f, -0.42f) * mechWorld;
            RenderJetPlume(leftShoulderMat,  shoulderLen, shoulderRad, outerColor, coreColor, view, proj);
            RenderJetPlume(rightShoulderMat, shoulderLen, shoulderRad, outerColor, coreColor, view, proj);
        }

        // 3. Foot Verniers (Pointing downwards -Y: rotated 90 degrees around X)
        if (footLen > 0.05f)
        {
            XMMATRIX leftFootMat  = XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(-0.35f, 0.05f, 0.10f) * mechWorld;
            XMMATRIX rightFootMat = XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation( 0.35f, 0.05f, 0.10f) * mechWorld;
            RenderJetPlume(leftFootMat,  footLen, footRad, outerColor, coreColor, view, proj);
            RenderJetPlume(rightFootMat, footLen, footRad, outerColor, coreColor, view, proj);
        }

        // Restore normal Opaque Blend and Standard Depth State
        m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    }

    void D3D11Renderer::RenderRemoteMechThrusters(const RemoteMech& remoteMech, const XMMATRIX& mechWorld, const XMMATRIX& view, const XMMATRIX& proj)
    {
        if (!remoteMech.IsAlive()) return;

        bool isKick = remoteMech.IsBoostKicking();
        bool isAB = remoteMech.IsAssaultBoost();
        bool isQB = remoteMech.IsQuickBoost();
        bool isBoost = remoteMech.IsBoostMode();

        if (!isKick && !isAB && !isQB && !isBoost) return;

        static uint32_t s_remoteFrame = 0;
        s_remoteFrame++;
        float flicker = 1.0f + 0.08f * std::sin(s_remoteFrame * 0.40f) + 0.05f * std::cos(s_remoteFrame * 0.85f);

        // Player theme colors for remote thrusters
        uint8_t pId = remoteMech.GetPlayerId() % 4;
        XMFLOAT4 playerJetColors[4] = {
            { 1.00f, 0.28f, 0.15f, 1.0f }, // P1: Crimson / Burning Amber
            { 0.20f, 0.85f, 1.00f, 1.0f }, // P2: Cobalt Cyan
            { 1.00f, 0.75f, 0.10f, 1.0f }, // P3: Amber Gold
            { 0.15f, 0.95f, 0.45f, 1.0f }  // P4: Emerald Green
        };

        XMFLOAT4 outerColor = playerJetColors[pId];
        XMFLOAT4 coreColor  = { 0.95f, 1.00f, 1.0f, 1.0f };

        float mainLen = 0.0f;
        float mainRad = 0.0f;
        float shoulderLen = 0.0f;
        float shoulderRad = 0.0f;
        float footLen = 0.0f;
        float footRad = 0.0f;

        if (isKick)
        {
            mainLen = (8.5f + 1.5f * std::sin(s_remoteFrame * 0.5f)) * flicker;
            mainRad = 0.55f * flicker;
            shoulderLen = 4.2f * flicker;
            shoulderRad = 0.32f * flicker;
            footLen = 3.2f * flicker;
            footRad = 0.28f * flicker;
            outerColor = { 1.0f, 0.25f, 0.05f, 1.0f }; // Ultra-hot Crimson/Orange
            coreColor  = { 1.0f, 1.0f, 0.9f, 1.0f };
        }
        else if (isAB)
        {
            mainLen = (6.8f + 1.2f * std::sin(s_remoteFrame * 0.35f)) * flicker;
            mainRad = 0.44f * flicker;
            shoulderLen = 3.6f * flicker;
            shoulderRad = 0.26f * flicker;
            footLen = 2.4f * flicker;
            footRad = 0.22f * flicker;
            outerColor = { 1.0f, 0.45f, 0.10f, 1.0f }; // AB overrides to blazing orange
            coreColor  = { 1.0f, 0.95f, 0.75f, 1.0f };
        }
        else if (isQB)
        {
            mainLen = 4.2f * flicker;
            mainRad = 0.38f * flicker;
            shoulderLen = 3.0f * flicker;
            shoulderRad = 0.28f * flicker;
            footLen = 1.8f * flicker;
            footRad = 0.20f * flicker;
            coreColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        }
        else if (isBoost)
        {
            XMFLOAT3 vel = remoteMech.GetVelocity();
            float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
            float speedFactor = std::clamp(speed / 55.0f, 0.0f, 1.0f);

            mainLen = (1.4f + speedFactor * 1.5f) * flicker;
            mainRad = (0.20f + speedFactor * 0.08f) * flicker;
            shoulderLen = (0.6f + speedFactor * 0.8f) * flicker;
            shoulderRad = (0.12f + speedFactor * 0.05f) * flicker;
            footLen = 0.8f * flicker;
            footRad = 0.14f * flicker;
        }

        // Set Additive Blending and Depth Test (Read Only)
        m_context->OMSetBlendState(m_additiveBlendState.Get(), nullptr, 0xFFFFFFFF);
        m_context->OMSetDepthStencilState(m_thrusterDepthState.Get(), 0);

        // 1. Back Main Thrusters
        if (mainLen > 0.05f)
        {
            XMMATRIX leftMainMat  = XMMatrixTranslation(-0.35f, 1.60f, -0.55f) * mechWorld;
            XMMATRIX rightMainMat = XMMatrixTranslation( 0.35f, 1.60f, -0.55f) * mechWorld;
            RenderJetPlume(leftMainMat,  mainLen, mainRad, outerColor, coreColor, view, proj);
            RenderJetPlume(rightMainMat, mainLen, mainRad, outerColor, coreColor, view, proj);
        }

        // 2. Shoulder Side Boosters
        if (shoulderLen > 0.05f)
        {
            XMMATRIX leftShoulderMat  = XMMatrixRotationY( 0.10f) * XMMatrixTranslation(-0.78f, 1.85f, -0.42f) * mechWorld;
            XMMATRIX rightShoulderMat = XMMatrixRotationY(-0.10f) * XMMatrixTranslation( 0.78f, 1.85f, -0.42f) * mechWorld;
            RenderJetPlume(leftShoulderMat,  shoulderLen, shoulderRad, outerColor, coreColor, view, proj);
            RenderJetPlume(rightShoulderMat, shoulderLen, shoulderRad, outerColor, coreColor, view, proj);
        }

        // 3. Foot Verniers
        if (footLen > 0.05f)
        {
            XMMATRIX leftFootMat  = XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(-0.35f, 0.05f, 0.10f) * mechWorld;
            XMMATRIX rightFootMat = XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation( 0.35f, 0.05f, 0.10f) * mechWorld;
            RenderJetPlume(leftFootMat,  footLen, footRad, outerColor, coreColor, view, proj);
            RenderJetPlume(rightFootMat, footLen, footRad, outerColor, coreColor, view, proj);
        }

        // Restore normal Opaque Blend and Standard Depth State
        m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    }

    void D3D11Renderer::RenderAIBots(const std::vector<AIBotMech>& aiBots, const XMMATRIX& view, const XMMATRIX& proj)
    {
        for (const auto& bot : aiBots)
        {
            if (!bot.IsAlive()) continue;

            XMFLOAT3 pos = bot.GetPosition();
            XMFLOAT3 rot = bot.GetRotation(); // Yaw, Pitch, Roll

            float abPitch = bot.IsBoostKicking() ? 0.42f : (bot.IsAssaultBoosting() ? 0.35f : 0.0f);
            XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(rot.y * 0.3f + abPitch, rot.x, rot.z);
            XMMATRIX transMat = XMMatrixTranslation(pos.x, pos.y, pos.z);
            XMMATRIX world = rotMat * transMat;

            D3D11_MAPPED_SUBRESOURCE mapped;
            if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
                cb->world = world;
                cb->view = view;
                cb->projection = proj;
                cb->customParams = bot.IsHitFlashing() ? XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) : XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
                m_context->Unmap(m_constantBuffer.Get(), 0);
            }

            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            ID3D11Buffer* vBuf = m_enemyMechVertexBuffer.Get();
            m_context->IASetVertexBuffers(0, 1, &vBuf, &stride, &offset);
            m_context->IASetIndexBuffer(m_mechIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            m_context->DrawIndexed(m_mechIndexCount, 0, 0);

            // Render AI Bot weapons (Right: Kinetic Rifle, Left: Beam Rifle, Left Back: Missile Pod)
            WeaponInstance rWp, lWp, lbWp;
            rWp.data.type = WeaponType::KineticRifle;
            lWp.data.type = WeaponType::BeamRifle;
            lbWp.data.type = WeaponType::MissilePod;

            XMMATRIX raMat = XMMatrixTranslation(0.95f, 1.0f, 0.40f) * world;
            RenderMechWeapon(rWp, raMat, view, proj);

            XMMATRIX laMat = XMMatrixTranslation(-0.95f, 1.0f, 0.40f) * world;
            RenderMechWeapon(lWp, laMat, view, proj);

            XMMATRIX lbMat = XMMatrixRotationRollPitchYaw(0.0f, 0.0f, 0.0f) * XMMatrixTranslation(-0.80f, 2.30f, -0.30f) * world;
            RenderMechWeapon(lbWp, lbMat, view, proj);

            // Thruster Jet Plumes for autonomous combat bot
            RenderAIBotThrusters(bot, world, view, proj);
        }
    }

    void D3D11Renderer::RenderAIBotThrusters(const AIBotMech& bot, const XMMATRIX& botWorld, const XMMATRIX& view, const XMMATRIX& proj)
    {
        if (bot.IsDestroyed()) return;

        bool isKick = bot.IsBoostKicking();
        bool isAB = bot.IsAssaultBoosting();
        bool isQB = bot.IsQuickBoosting();
        bool isBoost = bot.IsBoostMode();

        if (!isKick && !isAB && !isQB && !isBoost) return;

        static uint32_t s_botFrame = 0;
        s_botFrame++;
        float flicker = 1.0f + 0.08f * std::sin(s_botFrame * 0.50f) + 0.05f * std::cos(s_botFrame * 1.10f);

        float mainLen = 0.0f, mainRad = 0.0f;
        float shoulderLen = 0.0f, shoulderRad = 0.0f;
        float footLen = 0.0f, footRad = 0.0f;

        XMFLOAT4 outerColor = { 1.0f, 0.20f, 0.50f, 1.0f }; // Hot Magenta/Red
        XMFLOAT4 coreColor  = { 1.0f, 1.0f, 1.0f, 1.0f };

        if (isKick)
        {
            mainLen = (8.5f + 1.5f * std::sin(s_botFrame * 0.5f)) * flicker;
            mainRad = 0.55f * flicker;
            shoulderLen = 4.2f * flicker;
            shoulderRad = 0.32f * flicker;
            footLen = 3.2f * flicker;
            footRad = 0.28f * flicker;
            outerColor = { 1.0f, 0.25f, 0.05f, 1.0f };
            coreColor  = { 1.0f, 1.0f, 0.9f, 1.0f };
        }
        else if (isAB)
        {
            mainLen = (6.8f + 1.2f * std::sin(s_botFrame * 0.35f)) * flicker;
            mainRad = 0.44f * flicker;
            shoulderLen = 3.6f * flicker;
            shoulderRad = 0.26f * flicker;
            footLen = 2.4f * flicker;
            footRad = 0.22f * flicker;
            outerColor = { 1.0f, 0.45f, 0.10f, 1.0f };
            coreColor  = { 1.0f, 0.95f, 0.75f, 1.0f };
        }
        else if (isQB)
        {
            mainLen = 4.2f * flicker;
            mainRad = 0.38f * flicker;
            shoulderLen = 3.0f * flicker;
            shoulderRad = 0.28f * flicker;
            footLen = 1.8f * flicker;
            footRad = 0.20f * flicker;
            coreColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        }
        else if (isBoost)
        {
            XMFLOAT3 vel = bot.GetVelocity();
            float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);
            float speedFactor = std::clamp(speed / 55.0f, 0.0f, 1.0f);

            mainLen = (1.4f + speedFactor * 1.5f) * flicker;
            mainRad = (0.20f + speedFactor * 0.08f) * flicker;
            shoulderLen = (0.6f + speedFactor * 0.8f) * flicker;
            shoulderRad = (0.12f + speedFactor * 0.05f) * flicker;
            footLen = 0.8f * flicker;
            footRad = 0.14f * flicker;
        }

        m_context->OMSetBlendState(m_additiveBlendState.Get(), nullptr, 0xFFFFFFFF);
        m_context->OMSetDepthStencilState(m_thrusterDepthState.Get(), 0);

        if (mainLen > 0.05f)
        {
            XMMATRIX leftMainMat  = XMMatrixTranslation(-0.35f, 1.60f, -0.55f) * botWorld;
            XMMATRIX rightMainMat = XMMatrixTranslation( 0.35f, 1.60f, -0.55f) * botWorld;
            RenderJetPlume(leftMainMat,  mainLen, mainRad, outerColor, coreColor, view, proj);
            RenderJetPlume(rightMainMat, mainLen, mainRad, outerColor, coreColor, view, proj);
        }

        if (shoulderLen > 0.05f)
        {
            XMMATRIX leftShoulderMat  = XMMatrixRotationY( 0.10f) * XMMatrixTranslation(-0.78f, 1.85f, -0.42f) * botWorld;
            XMMATRIX rightShoulderMat = XMMatrixRotationY(-0.10f) * XMMatrixTranslation( 0.78f, 1.85f, -0.42f) * botWorld;
            RenderJetPlume(leftShoulderMat,  shoulderLen, shoulderRad, outerColor, coreColor, view, proj);
            RenderJetPlume(rightShoulderMat, shoulderLen, shoulderRad, outerColor, coreColor, view, proj);
        }

        if (footLen > 0.05f)
        {
            XMMATRIX leftFootMat  = XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(-0.35f, 0.05f, 0.10f) * botWorld;
            XMMATRIX rightFootMat = XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation( 0.35f, 0.05f, 0.10f) * botWorld;
            RenderJetPlume(leftFootMat,  footLen, footRad, outerColor, coreColor, view, proj);
            RenderJetPlume(rightFootMat, footLen, footRad, outerColor, coreColor, view, proj);
        }

        m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
    }

    void D3D11Renderer::RenderHUD(
        const MechController& mech,
        const WeaponSystem& weapons,
        const TargetLockSystem& lockSystem,
        CameraMode cameraMode,
        const NetworkManager* network,
        const std::vector<RemoteMech>* remoteMechs,
        const std::vector<TargetDummy>* targets,
        const Camera* camera,
        const std::vector<AIBotMech>* aiBots)
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
        dynRetVerts.reserve(4096);

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

        // Network Status Display (Top Center)
        if (network && network->GetMode() != NetworkRole::None)
        {
            std::string netStr = network->GetStatusString();
            float charH = 0.020f;
            float charW = charH * invAspect * 0.75f;
            float spacing = charW * 0.35f;
            float totalW = netStr.length() * (charW + spacing);
            float textX = -totalW * 0.5f;
            float textY = 0.88f;

            XMFLOAT4 netCol = network->IsConnected() ? XMFLOAT4(0.25f, 0.95f, 0.45f, 0.95f) : XMFLOAT4(1.0f, 0.85f, 0.25f, 0.95f);
            AddStringLines(netStr, textX, textY, charW, charH, spacing, netCol, dynRetVerts);
        }

        // Add 4-Slot Weapon Status & AP text labels in dynamic reticle buffer
        {
            float tH = 0.016f;
            float tW = tH * invAspect * 0.72f;
            float tSp = tW * 0.35f;

            // Weapon Slots Info (Bottom-Right)
            const auto& slotRA = weapons.GetSlot(WeaponSlot::RightArm);
            const auto& slotRB = weapons.GetSlot(WeaponSlot::RightBack);
            const auto& slotLA = weapons.GetSlot(WeaponSlot::LeftArm);
            const auto& slotLB = weapons.GetSlot(WeaponSlot::LeftBack);

            std::string rbAction = (slotRB.data.category == WeaponCategory::BackOnlyWeapon) ? " [E]SHOOT" : " [E]SWAP";
            std::string lbAction = (slotLB.data.category == WeaponCategory::BackOnlyWeapon) ? " [Q]SHOOT" : " [Q]SWAP";
            std::string raText = "RA " + slotRA.data.shortName + " " + (slotRA.isReloading ? "RELOAD" : std::to_string(slotRA.currentAmmo) + "/" + std::to_string(slotRA.data.maxAmmo));
            std::string rbText = "RB " + slotRB.data.shortName + " " + (slotRB.isReloading ? "RELOAD" : std::to_string(slotRB.currentAmmo) + "/" + std::to_string(slotRB.data.maxAmmo)) + rbAction;
            std::string laText = "LA " + slotLA.data.shortName + " " + (slotLA.isReloading ? "RELOAD" : std::to_string(slotLA.currentAmmo) + "/" + std::to_string(slotLA.data.maxAmmo));
            std::string lbText = "LB " + slotLB.data.shortName + " " + (slotLB.isReloading ? "RELOAD" : std::to_string(slotLB.currentAmmo) + "/" + std::to_string(slotLB.data.maxAmmo)) + lbAction;

            XMFLOAT4 raTextCol = slotRA.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.2f, 0.95f) : XMFLOAT4(1.0f, 0.65f, 0.20f, 0.90f);
            XMFLOAT4 rbTextCol = slotRB.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.2f, 0.95f) : XMFLOAT4(1.0f, 0.82f, 0.25f, 0.85f);
            XMFLOAT4 laTextCol = slotLA.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.2f, 0.95f) : XMFLOAT4(0.3f, 0.85f, 1.00f, 0.90f);
            XMFLOAT4 lbTextCol = slotLB.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.2f, 0.95f) : XMFLOAT4(0.88f, 0.35f, 1.00f, 0.85f);

            AddStringLines(raText, 0.52f, -0.638f, tW, tH, tSp, raTextCol, dynRetVerts);
            AddStringLines(rbText, 0.52f, -0.678f, tW, tH, tSp, rbTextCol, dynRetVerts);
            AddStringLines(laText, 0.52f, -0.728f, tW, tH, tSp, laTextCol, dynRetVerts);
            AddStringLines(lbText, 0.52f, -0.768f, tW, tH, tSp, lbTextCol, dynRetVerts);

            // AP Label (Bottom-Left)
            int curHp = static_cast<int>(mech.GetHp());
            int maxHp = static_cast<int>(mech.GetMaxHp());
            std::string apText = "AP " + std::to_string(curHp) + "/" + std::to_string(maxHp);
            XMFLOAT4 apTextCol = (mech.GetHpRatio() > 0.35f) ? XMFLOAT4(0.25f, 0.95f, 0.40f, 0.95f) : XMFLOAT4(1.0f, 0.25f, 0.20f, 0.95f);
            AddStringLines(apText, -0.85f, -0.710f, tW, tH, tSp, apTextCol, dynRetVerts);

            // ACS Label (Bottom-Left, below AP)
            int curAcs = static_cast<int>(mech.GetAcs());
            int maxAcs = static_cast<int>(mech.GetMaxAcs());
            std::string acsText = "ACS " + std::to_string(curAcs) + "/" + std::to_string(maxAcs);
            static float s_hudAcsBlink = 0.0f;
            s_hudAcsBlink += 0.016f;
            float acsBlinkAlpha = (std::sin(s_hudAcsBlink * 20.0f) > 0.0f) ? 1.0f : 0.35f;
            XMFLOAT4 acsTextCol = mech.IsStaggered() ? XMFLOAT4(1.0f, 0.15f, 0.10f, acsBlinkAlpha) : (mech.GetAcsRatio() > 0.70f ? XMFLOAT4(1.0f, 0.55f, 0.15f, 0.95f) : XMFLOAT4(1.0f, 0.85f, 0.25f, 0.90f));
            AddStringLines(acsText, -0.85f, -0.770f, tW, tH, tSp, acsTextCol, dynRetVerts);

            // Player STAGGER OVERLOAD Warning (Center Reticle Area)
            if (mech.IsStaggered())
            {
                std::string stgWarn = "! STAGGER OVERLOAD !";
                float stgH = 0.026f;
                float stgW = stgH * invAspect * 0.75f;
                float stgSp = stgW * 0.35f;
                float totalStgW = stgWarn.length() * (stgW + stgSp);
                float stgX = -totalStgW * 0.5f;
                float stgY = 0.20f;
                AddStringLines(stgWarn, stgX, stgY, stgW, stgH, stgSp, XMFLOAT4(1.0f, 0.15f, 0.10f, acsBlinkAlpha), dynRetVerts);
            }

            // Assemble status / Garage quick-access guide (Top-Left)
            std::string assembleGuide = "CUSTOM ASSEMBLE UNIT  [G] GARAGE";
            AddStringLines(assembleGuide, -0.92f, 0.88f, tW * 0.88f, tH * 0.88f, tSp * 0.88f, XMFLOAT4(0.35f, 0.85f, 1.0f, 0.90f), dynRetVerts);

            // Boost Kick / Assault Boost Status & Operation Guide
            if (mech.IsBoostKicking())
            {
                static float s_kickBlink = 0.0f;
                s_kickBlink += 0.016f;
                float kickAlpha = (std::sin(s_kickBlink * 30.0f) > 0.0f) ? 1.0f : 0.4f;
                std::string kickText = ">> BOOST KICK <<";
                float kH = 0.026f;
                float kW = kH * invAspect * 0.75f;
                float kSp = kW * 0.35f;
                float totalKW = static_cast<float>(kickText.length()) * (kW + kSp);
                AddStringLines(kickText, -totalKW * 0.5f, -0.15f, kW, kH, kSp, XMFLOAT4(1.0f, 0.40f, 0.10f, kickAlpha), dynRetVerts);
            }
            else if (mech.IsAssaultBoost())
            {
                std::string abGuide = "[CTRL / L3] BOOST KICK    [S] CANCEL AB";
                float abH = 0.018f;
                float abW = abH * invAspect * 0.72f;
                float abSp = abW * 0.35f;
                float totalAbW = static_cast<float>(abGuide.length()) * (abW + abSp);
                AddStringLines(abGuide, -totalAbW * 0.5f, -0.68f, abW, abH, abSp, XMFLOAT4(1.0f, 0.75f, 0.20f, 0.95f), dynRetVerts);
            }
        }

        // =============================================================
        // (A) Damage Direction Indicators (Hit Arcs around screen center)
        // =============================================================
        const auto& dmgInds = mech.GetDamageIndicators();
        for (const auto& ind : dmgInds)
        {
            if (ind.intensity <= 0.001f) continue;

            float baseRadius = 0.22f;
            // relativeAngle: 0 = straight ahead, PI/2 = right, PI = behind, -PI/2 = left
            // In screen polar space: angle 0 is +X (right), PI/2 is +Y (up)
            // Ahead (relAngle=0) -> screen Up (+Y), so phi = PI/2 - relAngle
            float centerPhi = XM_PIDIV2 - ind.relativeAngle;
            float span = XM_PI / 4.0f; // 45 degree arc
            int segCount = 10;
            float alpha = std::clamp(ind.intensity, 0.0f, 1.0f);
            XMFLOAT4 arcColOuter = { 1.0f, 0.18f, 0.15f, alpha * 0.95f };
            XMFLOAT4 arcColInner = { 1.0f, 0.55f, 0.20f, alpha * 0.95f };

            float rOuter = baseRadius * 1.05f;
            float rInner = baseRadius;

            // Draw outer & inner arc bands
            for (int s = 0; s < segCount; ++s)
            {
                float t1 = -0.5f + static_cast<float>(s) / segCount;
                float t2 = -0.5f + static_cast<float>(s + 1) / segCount;
                float a1 = centerPhi + t1 * span;
                float a2 = centerPhi + t2 * span;

                float x1_o = std::cos(a1) * rOuter * invAspect;
                float y1_o = std::sin(a1) * rOuter;
                float x2_o = std::cos(a2) * rOuter * invAspect;
                float y2_o = std::sin(a2) * rOuter;

                dynRetVerts.push_back({ { x1_o, y1_o, 0.0f }, { 0, 0, 1 }, arcColOuter });
                dynRetVerts.push_back({ { x2_o, y2_o, 0.0f }, { 0, 0, 1 }, arcColOuter });

                float x1_i = std::cos(a1) * rInner * invAspect;
                float y1_i = std::sin(a1) * rInner;
                float x2_i = std::cos(a2) * rInner * invAspect;
                float y2_i = std::sin(a2) * rInner;

                dynRetVerts.push_back({ { x1_i, y1_i, 0.0f }, { 0, 0, 1 }, arcColInner });
                dynRetVerts.push_back({ { x2_i, y2_i, 0.0f }, { 0, 0, 1 }, arcColInner });
            }

            // Draw sharp pointer arrow towards damage origin
            float tipR = rOuter * 1.10f;
            float sideR = rInner * 0.95f;
            float tipX = std::cos(centerPhi) * tipR * invAspect;
            float tipY = std::sin(centerPhi) * tipR;
            float side1X = std::cos(centerPhi - span * 0.22f) * sideR * invAspect;
            float side1Y = std::sin(centerPhi - span * 0.22f) * sideR;
            float side2X = std::cos(centerPhi + span * 0.22f) * sideR * invAspect;
            float side2Y = std::sin(centerPhi + span * 0.22f) * sideR;

            dynRetVerts.push_back({ { tipX, tipY, 0.0f }, { 0, 0, 1 }, arcColOuter });
            dynRetVerts.push_back({ { side1X, side1Y, 0.0f }, { 0, 0, 1 }, arcColOuter });
            dynRetVerts.push_back({ { tipX, tipY, 0.0f }, { 0, 0, 1 }, arcColOuter });
            dynRetVerts.push_back({ { side2X, side2Y, 0.0f }, { 0, 0, 1 }, arcColOuter });
        }

        // =============================================================
        // (B) Tactical Mini-Radar HUD (Top-Right AC6 Cockpit Scanner)
        // =============================================================
        float radarCx = 0.76f;
        float radarCy = 0.62f;
        float radarR  = 0.15f;
        float radarRange = 140.0f; // 140 meters detection range

        XMFLOAT4 radarRingCol  = { 0.15f, 0.50f, 0.75f, 0.65f };
        XMFLOAT4 radarInnerCol = { 0.12f, 0.35f, 0.55f, 0.35f };
        XMFLOAT4 radarCrossCol = { 0.15f, 0.45f, 0.65f, 0.40f };

        // Outer circular frame (24 segments)
        int rSegs = 24;
        for (int s = 0; s < rSegs; ++s)
        {
            float a1 = s * (XM_2PI / rSegs);
            float a2 = (s + 1) * (XM_2PI / rSegs);
            float x1 = radarCx + std::cos(a1) * radarR * invAspect;
            float y1 = radarCy + std::sin(a1) * radarR;
            float x2 = radarCx + std::cos(a2) * radarR * invAspect;
            float y2 = radarCy + std::sin(a2) * radarR;
            dynRetVerts.push_back({ { x1, y1, 0.0f }, { 0, 0, 1 }, radarRingCol });
            dynRetVerts.push_back({ { x2, y2, 0.0f }, { 0, 0, 1 }, radarRingCol });

            // Mid range ring (70m = 0.50)
            float rMid = radarR * 0.50f;
            float mx1 = radarCx + std::cos(a1) * rMid * invAspect;
            float my1 = radarCy + std::sin(a1) * rMid;
            float mx2 = radarCx + std::cos(a2) * rMid * invAspect;
            float my2 = radarCy + std::sin(a2) * rMid;
            dynRetVerts.push_back({ { mx1, my1, 0.0f }, { 0, 0, 1 }, radarInnerCol });
            dynRetVerts.push_back({ { mx2, my2, 0.0f }, { 0, 0, 1 }, radarInnerCol });
        }

        // Crosshairs on Radar
        dynRetVerts.push_back({ { radarCx - radarR * invAspect, radarCy, 0.0f }, { 0, 0, 1 }, radarCrossCol });
        dynRetVerts.push_back({ { radarCx + radarR * invAspect, radarCy, 0.0f }, { 0, 0, 1 }, radarCrossCol });
        dynRetVerts.push_back({ { radarCx, radarCy - radarR, 0.0f }, { 0, 0, 1 }, radarCrossCol });
        dynRetVerts.push_back({ { radarCx, radarCy + radarR, 0.0f }, { 0, 0, 1 }, radarCrossCol });

        // Center Mech Player Arrow (facing Up on radar)
        float pArrW = 0.007f * invAspect;
        float pArrH = 0.012f;
        XMFLOAT4 playerArrCol = { 1.0f, 1.0f, 1.0f, 0.95f };
        dynRetVerts.push_back({ { radarCx, radarCy + pArrH, 0.0f }, { 0, 0, 1 }, playerArrCol });
        dynRetVerts.push_back({ { radarCx - pArrW, radarCy - pArrH * 0.5f, 0.0f }, { 0, 0, 1 }, playerArrCol });
        dynRetVerts.push_back({ { radarCx, radarCy + pArrH, 0.0f }, { 0, 0, 1 }, playerArrCol });
        dynRetVerts.push_back({ { radarCx + pArrW, radarCy - pArrH * 0.5f, 0.0f }, { 0, 0, 1 }, playerArrCol });
        dynRetVerts.push_back({ { radarCx - pArrW, radarCy - pArrH * 0.5f, 0.0f }, { 0, 0, 1 }, playerArrCol });
        dynRetVerts.push_back({ { radarCx + pArrW, radarCy - pArrH * 0.5f, 0.0f }, { 0, 0, 1 }, playerArrCol });

        // Radar Label
        std::string radarLabel = "RADAR 140M";
        float rLabH = 0.012f;
        float rLabW = rLabH * invAspect * 0.70f;
        float rLabSp = rLabW * 0.35f;
        float rLabTotalW = radarLabel.length() * (rLabW + rLabSp);
        AddStringLines(radarLabel, radarCx - rLabTotalW * 0.5f, radarCy + radarR + 0.018f, rLabW, rLabH, rLabSp, radarRingCol, dynRetVerts);

        // Rotating Sweeper Line
        static float s_radarAngle = 0.0f;
        s_radarAngle += 0.035f;
        if (s_radarAngle > XM_2PI) s_radarAngle -= XM_2PI;
        float swX = radarCx + std::cos(s_radarAngle) * radarR * invAspect;
        float swY = radarCy + std::sin(s_radarAngle) * radarR;
        XMFLOAT4 sweepCol = { 0.20f, 0.75f, 0.95f, 0.40f };
        dynRetVerts.push_back({ { radarCx, radarCy, 0.0f }, { 0, 0, 1 }, sweepCol });
        dynRetVerts.push_back({ { swX, swY, 0.0f }, { 0, 0, 1 }, sweepCol });

        // Plot Remote Mechs & Target Dummies on Radar
        XMFLOAT3 myPos = mech.GetPosition();
        float myYaw = mech.GetYaw();
        float sinY = std::sin(myYaw);
        float cosY = std::cos(myYaw);

        auto plotRadarBlip = [&](const XMFLOAT3& worldPos, const XMFLOAT4& col, bool isDestroyed) {
            float dx = worldPos.x - myPos.x;
            float dz = worldPos.z - myPos.z;
            float dy = worldPos.y - myPos.y;

            // Transform into local mech orientation (Facing Up on radar: +Z is local forward)
            float lx = dx * cosY - dz * sinY;
            float ly = dx * sinY + dz * cosY;
            float dist = std::sqrt(lx * lx + ly * ly);

            float nx = lx / radarRange;
            float ny = ly / radarRange;
            float nd = std::sqrt(nx * nx + ny * ny);
            if (nd > 1.0f)
            {
                nx /= nd;
                ny /= nd;
            }

            float bx = radarCx + nx * radarR * invAspect;
            float by = radarCy + ny * radarR;
            float bs = 0.007f;
            float bsX = bs * invAspect;

            if (isDestroyed)
            {
                // Cross mark (X)
                dynRetVerts.push_back({ { bx - bsX, by - bs, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx + bsX, by + bs, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx - bsX, by + bs, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx + bsX, by - bs, 0.0f }, { 0, 0, 1 }, col });
            }
            else if (dy > 3.5f)
            {
                // Above me: Up arrow
                dynRetVerts.push_back({ { bx, by + bs * 1.3f, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx - bsX, by - bs * 0.7f, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx, by + bs * 1.3f, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx + bsX, by - bs * 0.7f, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx - bsX, by - bs * 0.7f, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx + bsX, by - bs * 0.7f, 0.0f }, { 0, 0, 1 }, col });
            }
            else if (dy < -3.5f)
            {
                // Below me: Down arrow
                dynRetVerts.push_back({ { bx, by - bs * 1.3f, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx - bsX, by + bs * 0.7f, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx, by - bs * 1.3f, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx + bsX, by + bs * 0.7f, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx - bsX, by + bs * 0.7f, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx + bsX, by + bs * 0.7f, 0.0f }, { 0, 0, 1 }, col });
            }
            else
            {
                // Level height: Diamond
                dynRetVerts.push_back({ { bx, by + bs, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx + bsX, by, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx + bsX, by, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx, by - bs, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx, by - bs, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx - bsX, by, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx - bsX, by, 0.0f }, { 0, 0, 1 }, col });
                dynRetVerts.push_back({ { bx, by + bs, 0.0f }, { 0, 0, 1 }, col });
            }
        };

        if (remoteMechs)
        {
            XMFLOAT4 pThemeCols[4] = {
                { 1.00f, 0.25f, 0.20f, 0.95f }, // P1: Crimson
                { 0.25f, 0.85f, 1.00f, 0.95f }, // P2: Cobalt Cyan
                { 1.00f, 0.80f, 0.15f, 0.95f }, // P3: Amber Gold
                { 0.20f, 0.95f, 0.45f, 0.95f }  // P4: Emerald Green
            };

            for (const auto& rMech : *remoteMechs)
            {
                if (!rMech.IsActive()) continue;
                uint8_t pId = rMech.GetPlayerId() % 4;
                XMFLOAT4 col = rMech.IsDestroyed() ? XMFLOAT4(0.6f, 0.6f, 0.6f, 0.6f) : pThemeCols[pId];
                plotRadarBlip(rMech.GetPosition(), col, rMech.IsDestroyed());
            }
        }

        if (targets)
        {
            XMFLOAT4 dummyCol = { 1.0f, 0.60f, 0.20f, 0.80f };
            for (const auto& t : *targets)
            {
                if (t.IsDestroyed()) continue;
                plotRadarBlip(t.GetPosition(), dummyCol, false);
            }
        }

        if (aiBots)
        {
            XMFLOAT4 botRadarCol = { 1.0f, 0.20f, 0.50f, 0.95f };
            for (const auto& b : *aiBots)
            {
                if (!b.IsActive()) continue;
                XMFLOAT4 col = b.IsDestroyed() ? XMFLOAT4(0.6f, 0.6f, 0.6f, 0.6f) : botRadarCol;
                plotRadarBlip(b.GetPosition(), col, b.IsDestroyed());
            }
        }

        // =============================================================
        // (C) Off-Screen Edge Arrow Indicators for Remote Enemies
        // =============================================================
        if (remoteMechs && camera)
        {
            XMMATRIX viewMat = camera->GetViewMatrix();
            XMMATRIX projMat = camera->GetProjectionMatrix(aspect);
            XMMATRIX vp = XMMatrixMultiply(viewMat, projMat);

            for (const auto& rMech : *remoteMechs)
            {
                if (!rMech.IsActive() || rMech.IsDestroyed()) continue;

                XMFLOAT3 rPos = rMech.GetPosition();
                rPos.y += 1.0f;
                XMVECTOR worldV = XMVectorSet(rPos.x, rPos.y, rPos.z, 1.0f);
                XMVECTOR clipV  = XMVector4Transform(worldV, vp);
                XMFLOAT4 clip;
                XMStoreFloat4(&clip, clipV);

                bool isBehind = (clip.w <= 0.1f);
                float ndcX = isBehind ? -clip.x : (clip.x / clip.w);
                float ndcY = isBehind ? -clip.y : (clip.y / clip.w);

                // Check if target is outside screen viewing boundary
                float bX = 0.88f;
                float bY = 0.82f;
                bool isOffScreen = isBehind || (std::abs(ndcX) > bX || std::abs(ndcY) > bY);

                if (isOffScreen)
                {
                    float dirLen = std::sqrt(ndcX * ndcX + ndcY * ndcY);
                    if (dirLen > 0.001f)
                    {
                        float scaleX = bX / std::max(0.001f, std::abs(ndcX));
                        float scaleY = bY / std::max(0.001f, std::abs(ndcY));
                        float sMin = std::min(scaleX, scaleY);
                        float edgeX = ndcX * sMin;
                        float edgeY = ndcY * sMin;

                        float ux = edgeX / dirLen;
                        float uy = edgeY / dirLen;

                        float px = -uy * invAspect;
                        float py = ux;

                        float arrLen = 0.024f;
                        float arrSpan = 0.016f;
                        float tipX = edgeX + ux * arrLen * invAspect;
                        float tipY = edgeY + uy * arrLen;
                        float s1X  = edgeX - ux * (arrLen * 0.4f) * invAspect + px * arrSpan;
                        float s1Y  = edgeY - uy * (arrLen * 0.4f) + py * arrSpan;
                        float s2X  = edgeX - ux * (arrLen * 0.4f) * invAspect - px * arrSpan;
                        float s2Y  = edgeY - uy * (arrLen * 0.4f) - py * arrSpan;

                        uint8_t pId = rMech.GetPlayerId() % 4;
                        XMFLOAT4 pThemeCols[4] = {
                            { 1.00f, 0.25f, 0.20f, 0.95f },
                            { 0.25f, 0.85f, 1.00f, 0.95f },
                            { 1.00f, 0.80f, 0.15f, 0.95f },
                            { 0.20f, 0.95f, 0.45f, 0.95f }
                        };
                        XMFLOAT4 arrowCol = pThemeCols[pId];

                        dynRetVerts.push_back({ { tipX, tipY, 0.0f }, { 0, 0, 1 }, arrowCol });
                        dynRetVerts.push_back({ { s1X, s1Y, 0.0f }, { 0, 0, 1 }, arrowCol });
                        dynRetVerts.push_back({ { tipX, tipY, 0.0f }, { 0, 0, 1 }, arrowCol });
                        dynRetVerts.push_back({ { s2X, s2Y, 0.0f }, { 0, 0, 1 }, arrowCol });

                        // Distance readout near edge arrow
                        float dx = rPos.x - myPos.x;
                        float dy = rPos.y - myPos.y;
                        float dz = rPos.z - myPos.z;
                        int dVal = static_cast<int>(std::sqrt(dx*dx + dy*dy + dz*dz));
                        std::string dStr = std::to_string(dVal) + "M";

                        float dH = 0.013f;
                        float dW = dH * invAspect * 0.70f;
                        float dSp = dW * 0.35f;
                        float totalW = dStr.length() * (dW + dSp);
                        float txtX = edgeX - ux * 0.040f * invAspect - totalW * 0.5f;
                        float txtY = edgeY - uy * 0.040f - dH * 0.5f;

                        AddStringLines(dStr, txtX, txtY, dW, dH, dSp, arrowCol, dynRetVerts);
                    }
                }
            }
        }

        // Off-Screen Edge Arrow Indicators for Autonomous AI Combat Bots
        if (aiBots && camera)
        {
            XMMATRIX viewMat = camera->GetViewMatrix();
            XMMATRIX projMat = camera->GetProjectionMatrix(aspect);
            XMMATRIX vp = XMMatrixMultiply(viewMat, projMat);

            for (const auto& bot : *aiBots)
            {
                if (!bot.IsActive() || bot.IsDestroyed()) continue;

                XMFLOAT3 bPos = bot.GetPosition();
                bPos.y += 1.0f;
                XMVECTOR worldV = XMVectorSet(bPos.x, bPos.y, bPos.z, 1.0f);
                XMVECTOR clipV  = XMVector4Transform(worldV, vp);
                XMFLOAT4 clip;
                XMStoreFloat4(&clip, clipV);

                bool isBehind = (clip.w <= 0.1f);
                float ndcX = isBehind ? -clip.x : (clip.x / clip.w);
                float ndcY = isBehind ? -clip.y : (clip.y / clip.w);

                float bX = 0.88f;
                float bY = 0.82f;
                bool isOffScreen = isBehind || (std::abs(ndcX) > bX || std::abs(ndcY) > bY);

                if (isOffScreen)
                {
                    float dirLen = std::sqrt(ndcX * ndcX + ndcY * ndcY);
                    if (dirLen > 0.001f)
                    {
                        float scaleX = bX / std::max(0.001f, std::abs(ndcX));
                        float scaleY = bY / std::max(0.001f, std::abs(ndcY));
                        float tEdge = std::min(scaleX, scaleY);

                        float edgeX = ndcX * tEdge;
                        float edgeY = ndcY * tEdge;

                        float ux = ndcX / dirLen;
                        float uy = ndcY / dirLen;

                        float px = -uy;
                        float py =  ux;

                        float arrowLen = 0.040f;
                        float arrowWidth = 0.022f;

                        float tipX = edgeX;
                        float tipY = edgeY;
                        float baseCX = edgeX - ux * arrowLen * invAspect;
                        float baseCY = edgeY - uy * arrowLen;

                        float leftX  = baseCX + px * arrowWidth * invAspect;
                        float leftY  = baseCY + py * arrowWidth;
                        float rightX = baseCX - px * arrowWidth * invAspect;
                        float rightY = baseCY - py * arrowWidth;

                        XMFLOAT4 arrowCol = { 1.0f, 0.20f, 0.50f, 0.95f }; // Hot Magenta/Red for Combat Bot

                        dynRetVerts.push_back({ { tipX, tipY, 0.0f }, { 0, 0, 1 }, arrowCol });
                        dynRetVerts.push_back({ { leftX, leftY, 0.0f }, { 0, 0, 1 }, arrowCol });
                        dynRetVerts.push_back({ { tipX, tipY, 0.0f }, { 0, 0, 1 }, arrowCol });
                        dynRetVerts.push_back({ { rightX, rightY, 0.0f }, { 0, 0, 1 }, arrowCol });
                        dynRetVerts.push_back({ { leftX, leftY, 0.0f }, { 0, 0, 1 }, arrowCol });
                        dynRetVerts.push_back({ { rightX, rightY, 0.0f }, { 0, 0, 1 }, arrowCol });

                        XMFLOAT3 myPos = mech.GetPosition();
                        float dist = std::sqrt((bPos.x - myPos.x) * (bPos.x - myPos.x) +
                                               (bPos.y - myPos.y) * (bPos.y - myPos.y) +
                                               (bPos.z - myPos.z) * (bPos.z - myPos.z));
                        std::string dStr = "BOT" + std::to_string(bot.GetBotId() + 1) + ":" + std::to_string(static_cast<int>(dist)) + "M";

                        float dH = 0.013f;
                        float dW = dH * invAspect * 0.70f;
                        float dSp = dW * 0.35f;
                        float totalW = dStr.length() * (dW + dSp);
                        float txtX = edgeX - ux * 0.040f * invAspect - totalW * 0.5f;
                        float txtY = edgeY - uy * 0.040f - dH * 0.5f;

                        AddStringLines(dStr, txtX, txtY, dW, dH, dSp, arrowCol, dynRetVerts);
                    }
                }
            }
        }

        // Arena Mode Notification Banner (top-center screen)
        if (m_bannerTimer > 0.0f && !m_bannerText.empty())
        {
            m_bannerTimer -= 0.016f;
            float alpha = std::min(1.0f, m_bannerTimer / 0.4f);
            XMFLOAT4 bannerCyan = { 0.20f, 0.90f, 1.0f, 0.95f * alpha };

            float bY = 0.78f;
            float bH = 0.08f;
            float bCharH = 0.032f;
            float bCharW = bCharH * invAspect * 0.75f;
            float bSpacing = bCharW * 0.35f;
            float textLen = static_cast<float>(m_bannerText.length());
            float totalW = textLen * (bCharW + bSpacing) + 0.12f;
            float bLeft = -totalW * 0.5f;
            float bRight = totalW * 0.5f;

            // Frame lines
            dynRetVerts.push_back({ { bLeft, bY + bH * 0.5f, 0.0f }, { 0, 0, 1 }, bannerCyan });
            dynRetVerts.push_back({ { bRight, bY + bH * 0.5f, 0.0f }, { 0, 0, 1 }, bannerCyan });
            dynRetVerts.push_back({ { bLeft, bY - bH * 0.5f, 0.0f }, { 0, 0, 1 }, bannerCyan });
            dynRetVerts.push_back({ { bRight, bY - bH * 0.5f, 0.0f }, { 0, 0, 1 }, bannerCyan });
            dynRetVerts.push_back({ { bLeft, bY + bH * 0.5f, 0.0f }, { 0, 0, 1 }, bannerCyan });
            dynRetVerts.push_back({ { bLeft, bY - bH * 0.5f, 0.0f }, { 0, 0, 1 }, bannerCyan });
            dynRetVerts.push_back({ { bRight, bY + bH * 0.5f, 0.0f }, { 0, 0, 1 }, bannerCyan });
            dynRetVerts.push_back({ { bRight, bY - bH * 0.5f, 0.0f }, { 0, 0, 1 }, bannerCyan });

            // Corner accent marks
            float cAcc = 0.015f;
            dynRetVerts.push_back({ { bLeft - cAcc * invAspect, bY + bH * 0.5f + cAcc, 0.0f }, { 0, 0, 1 }, bannerCyan });
            dynRetVerts.push_back({ { bLeft, bY + bH * 0.5f, 0.0f }, { 0, 0, 1 }, bannerCyan });
            dynRetVerts.push_back({ { bRight + cAcc * invAspect, bY + bH * 0.5f + cAcc, 0.0f }, { 0, 0, 1 }, bannerCyan });
            dynRetVerts.push_back({ { bRight, bY + bH * 0.5f, 0.0f }, { 0, 0, 1 }, bannerCyan });

            // Text
            float curX = -textLen * (bCharW + bSpacing) * 0.5f + bCharW * 0.5f;
            for (char c : m_bannerText)
            {
                AddCharLines(c, curX, bY, bCharW, bCharH, bannerCyan, dynRetVerts);
                curX += bCharW + bSpacing;
            }
        }

        if (!dynRetVerts.empty())
        {
            UINT copyCount = static_cast<UINT>(std::min(dynRetVerts.size(), size_t(4096)));
            if (SUCCEEDED(m_context->Map(m_dynamicReticleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                memcpy(mapped.pData, dynRetVerts.data(), sizeof(Vertex) * copyCount);
                m_context->Unmap(m_dynamicReticleBuffer.Get(), 0);
            }

            m_context->IASetVertexBuffers(0, 1, m_dynamicReticleBuffer.GetAddressOf(), &stride, &offset);
            m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            m_context->Draw(copyCount, 0);
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

        // 4. Update and Draw 4-Slot Weapon Ammo Bars (RA, RB, LA, LB in bottom-right)
        const auto& sRA = weapons.GetSlot(WeaponSlot::RightArm);
        const auto& sRB = weapons.GetSlot(WeaponSlot::RightBack);
        const auto& sLA = weapons.GetSlot(WeaponSlot::LeftArm);
        const auto& sLB = weapons.GetSlot(WeaponSlot::LeftBack);

        float raRatio = sRA.GetAmmoRatio();
        float rbRatio = sRB.GetAmmoRatio();
        float laRatio = sLA.GetAmmoRatio();
        float lbRatio = sLB.GetAmmoRatio();

        // Pulsing animation for reloading arms
        static float s_hudPulseTime = 0.0f;
        s_hudPulseTime += 0.016f;
        float pulseAlpha = 0.60f + 0.40f * std::sin(s_hudPulseTime * 14.0f);

        XMFLOAT4 raColor = sRA.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.15f, pulseAlpha) : XMFLOAT4(1.0f, 0.60f, 0.20f, 0.95f);
        XMFLOAT4 rbColor = sRB.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.15f, pulseAlpha) : XMFLOAT4(1.0f, 0.80f, 0.22f, 0.90f);
        XMFLOAT4 laColor = sLA.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.15f, pulseAlpha) : XMFLOAT4(0.25f, 0.85f, 1.00f, 0.95f);
        XMFLOAT4 lbColor = sLB.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.15f, pulseAlpha) : XMFLOAT4(0.88f, 0.25f, 1.00f, 0.90f);

        float barBgColor[4] = { 0.16f, 0.18f, 0.22f, 0.65f };
        XMFLOAT4 bgFrame = { barBgColor[0], barBgColor[1], barBgColor[2], barBgColor[3] };

        float aLeft = 0.52f, aRight = 0.88f;
        float raTop = -0.650f, raBot = -0.666f;
        float rbTop = -0.690f, rbBot = -0.706f;
        float laTop = -0.740f, laBot = -0.756f;
        float lbTop = -0.780f, lbBot = -0.796f;

        float raFill = aLeft + (aRight - aLeft) * raRatio;
        float rbFill = aLeft + (aRight - aLeft) * rbRatio;
        float laFill = aLeft + (aRight - aLeft) * laRatio;
        float lbFill = aLeft + (aRight - aLeft) * lbRatio;

        // 5. AP (Armor Points / Health) Bar in bottom-left
        float apRatio = mech.GetHpRatio();
        float apLeft = -0.85f, apRight = -0.55f;
        float apTop = -0.730f, apBot = -0.755f;
        float apFill = apLeft + (apRight - apLeft) * std::clamp(apRatio, 0.0f, 1.0f);
        XMFLOAT4 apColor = (apRatio > 0.35f) ? XMFLOAT4(0.25f, 0.95f, 0.40f, 0.95f) : XMFLOAT4(1.0f, 0.25f, 0.20f, 0.95f);

        // 6. ACS (Attitude Control System) Bar in bottom-left
        float acsRatio = mech.GetAcsRatio();
        float acsLeft = -0.85f, acsRight = -0.55f;
        float acsTop = -0.790f, acsBot = -0.810f;
        float acsFill = acsLeft + (acsRight - acsLeft) * std::clamp(acsRatio, 0.0f, 1.0f);
        static float s_barStgBlink = 0.0f;
        s_barStgBlink += 0.016f;
        float bStgAlpha = (std::sin(s_barStgBlink * 20.0f) > 0.0f) ? 1.0f : 0.35f;
        XMFLOAT4 acsColor = mech.IsStaggered() ? XMFLOAT4(1.0f, 0.15f, 0.10f, bStgAlpha) : (acsRatio > 0.70f ? XMFLOAT4(1.0f, 0.50f, 0.15f, 0.95f) : XMFLOAT4(1.0f, 0.80f, 0.25f, 0.90f));

        std::vector<Vertex> barVerts;
        barVerts.reserve(96);

        auto addBarQuad = [&](float l, float r, float t, float b, XMFLOAT4 col) {
            barVerts.push_back({ { l, b, 0.0f }, { 0, 0, 1 }, col });
            barVerts.push_back({ { l, t, 0.0f }, { 0, 0, 1 }, col });
            barVerts.push_back({ { r, t, 0.0f }, { 0, 0, 1 }, col });
            barVerts.push_back({ { l, b, 0.0f }, { 0, 0, 1 }, col });
            barVerts.push_back({ { r, t, 0.0f }, { 0, 0, 1 }, col });
            barVerts.push_back({ { r, b, 0.0f }, { 0, 0, 1 }, col });
        };

        // RA Frame & Fill
        addBarQuad(aLeft, aRight, raTop, raBot, bgFrame);
        addBarQuad(aLeft, raFill, raTop, raBot, raColor);

        // RB Frame & Fill
        addBarQuad(aLeft, aRight, rbTop, rbBot, bgFrame);
        addBarQuad(aLeft, rbFill, rbTop, rbBot, rbColor);

        // LA Frame & Fill
        addBarQuad(aLeft, aRight, laTop, laBot, bgFrame);
        addBarQuad(aLeft, laFill, laTop, laBot, laColor);

        // LB Frame & Fill
        addBarQuad(aLeft, aRight, lbTop, lbBot, bgFrame);
        addBarQuad(aLeft, lbFill, lbTop, lbBot, lbColor);

        // AP Frame & Fill
        addBarQuad(apLeft, apRight, apTop, apBot, bgFrame);
        addBarQuad(apLeft, apFill, apTop, apBot, apColor);

        // ACS Frame & Fill
        addBarQuad(acsLeft, acsRight, acsTop, acsBot, bgFrame);
        addBarQuad(acsLeft, acsFill, acsTop, acsBot, acsColor);

        if (SUCCEEDED(m_context->Map(m_dynamicAmmoBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            UINT copyCount = static_cast<UINT>(std::min(barVerts.size(), size_t(256)));
            memcpy(mapped.pData, barVerts.data(), sizeof(Vertex) * copyCount);
            m_context->Unmap(m_dynamicAmmoBuffer.Get(), 0);
        }

        m_context->IASetVertexBuffers(0, 1, m_dynamicAmmoBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->Draw(static_cast<UINT>(barVerts.size()), 0);
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
        bool isLeftEye,
        const std::vector<RemoteMech>* remoteMechs,
        const NetworkManager* network,
        const FrameSystem* frames,
        const std::vector<AIBotMech>* aiBots)
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

        // 3. Render Remote Enemy Mechs
        if (remoteMechs)
        {
            for (const auto& rMech : *remoteMechs)
            {
                RenderEnemyMech(rMech, view, proj);
            }
        }

        // 4. Render Autonomous Combat AI Bots
        if (aiBots)
        {
            RenderAIBots(*aiBots, view, proj);
        }

        // 5. Render 3D Holographic Lock-On Reticles directly on enemy bodies (Stereo VR compatible)
        XMFLOAT3 cockpitHeadPos = mech.GetCockpitHeadPosition();
        RenderTargetReticles(targets, lockSystem, view, proj, cockpitHeadPos, remoteMechs, aiBots);

        // 6. Render Mech (VR is always first-person cockpit, so isFPV=true draws only arms/weapons)
        RenderMech(mech, weapons, frames, view, proj, true);

        // 7. Render Projectiles
        RenderProjectiles(weapons, view, proj);

        // 8. Render 3D Holographic Cockpit HUD (Reticle, Lock-on marker, EN, Ammo, AP bars, Network status)
        RenderVRHUD(mech, weapons, lockSystem, view, proj, network);
    }

    void D3D11Renderer::RenderVRHUD(
        const MechController& mech,
        const WeaponSystem& weapons,
        const TargetLockSystem& lockSystem,
        const XMMATRIX& view,
        const XMMATRIX& proj,
        const NetworkManager* network)
    {
        m_context->OMSetDepthStencilState(m_hudDepthDisabledState.Get(), 0);

        XMFLOAT3 cockpitPos = mech.GetCockpitHeadPosition();
        float yaw = mech.GetYaw();
        float pitch = mech.GetPitch();

        // Holographic canopy HUD placed 2.2m ahead of the pilot's head
        float sinY = std::sin(yaw);
        float cosY = std::cos(yaw);
        float forwardX = sinY;
        float forwardZ = cosY;
        float rightX = cosY;
        float rightZ = -sinY;

        float hudDist = 2.2f;
        XMFLOAT3 hudCenter = {
            cockpitPos.x + forwardX * hudDist,
            cockpitPos.y + 0.15f,
            cockpitPos.z + forwardZ * hudDist
        };

        XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(pitch * 0.5f, yaw, 0.0f);
        XMMATRIX transMat = XMMatrixTranslation(hudCenter.x, hudCenter.y, hudCenter.z);
        XMMATRIX hudWorld = rotMat * transMat;

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = hudWorld;
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // Emissive unlit
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        m_context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        m_context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        m_context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
        m_context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        m_context->IASetInputLayout(m_inputLayout.Get());

        UINT stride = sizeof(Vertex);
        UINT offset = 0;

        // 1. Static Canopy Outer Reticle
        m_context->IASetVertexBuffers(0, 1, m_hudVertexBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        m_context->Draw(m_reticleVertexCount, 0);

        // 2. Dynamic Inner Reticle & Distance Meter in VR Cockpit
        const auto& targetInfo = lockSystem.GetCurrentTarget();
        XMFLOAT2 retNdc = lockSystem.GetInnerReticlePos();
        bool isHardLock = lockSystem.IsHardLockEnabled();

        std::vector<Vertex> dynRetVerts;
        dynRetVerts.reserve(384);

        XMFLOAT4 retColor = targetInfo.isAimed ? XMFLOAT4(1.0f, 0.18f, 0.15f, 0.95f) : XMFLOAT4(1.0f, 0.85f, 0.22f, 0.95f);
        XMFLOAT4 retYellow = { 1.0f, 0.88f, 0.25f, 0.95f };
        XMFLOAT4 distWhite = { 0.92f, 0.92f, 0.96f, 0.90f };

        float invAspect = 1.0f; // In 3D space, coordinate scaling is symmetric

        if (targetInfo.hasTarget || isHardLock)
        {
            float rx = retNdc.x * 0.35f;
            float ry = retNdc.y * 0.35f;

            // Center small crosshair
            float cw = 0.008f * invAspect;
            float ch = 0.008f;
            dynRetVerts.push_back({ { rx - cw, ry, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx + cw, ry, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx, ry - ch, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx, ry + ch, 0.0f }, { 0, 0, 1 }, retColor });

            // 4-corner brackets
            float bw = 0.030f * invAspect;
            float bh = 0.030f;
            float blenX = 0.010f * invAspect;
            float blenY = 0.010f;

            dynRetVerts.push_back({ { rx - bw, ry + bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx - bw + blenX, ry + bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx - bw, ry + bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx - bw, ry + bh - blenY, 0.0f }, { 0, 0, 1 }, retColor });

            dynRetVerts.push_back({ { rx + bw, ry + bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx + bw - blenX, ry + bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx + bw, ry + bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx + bw, ry + bh - blenY, 0.0f }, { 0, 0, 1 }, retColor });

            dynRetVerts.push_back({ { rx - bw, ry - bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx - bw + blenX, ry - bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx - bw, ry - bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx - bw, ry - bh + blenY, 0.0f }, { 0, 0, 1 }, retColor });

            dynRetVerts.push_back({ { rx + bw, ry - bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx + bw - blenX, ry - bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx + bw, ry - bh, 0.0f }, { 0, 0, 1 }, retColor });
            dynRetVerts.push_back({ { rx + bw, ry - bh + blenY, 0.0f }, { 0, 0, 1 }, retColor });

            if (targetInfo.hasTarget)
            {
                int distInt = static_cast<int>(std::round(targetInfo.distance));
                std::string distStr = "[ " + std::to_string(distInt) + "M ]";
                float textStartX = rx - 0.038f * invAspect;
                float textStartY = ry - bh - 0.022f;
                AddStringLines(distStr, textStartX, textStartY, 0.007f * invAspect, 0.013f, 0.003f * invAspect, distWhite, dynRetVerts);
            }
        }

        if (isHardLock)
        {
            std::string assistStr = "TARGET ASSIST";
            float tw = 0.006f * invAspect;
            float th = 0.012f;
            float sp = 0.003f * invAspect;
            float totalW = static_cast<float>(assistStr.length()) * (tw + sp);
            AddStringLines(assistStr, -totalW * 0.5f, 0.22f, tw, th, sp, retYellow, dynRetVerts);
        }

        if (network && network->GetMode() != NetworkRole::None)
        {
            std::string netStr = network->GetStatusString();
            float tw = 0.0055f * invAspect;
            float th = 0.011f;
            float sp = 0.0025f * invAspect;
            float totalW = static_cast<float>(netStr.length()) * (tw + sp);
            XMFLOAT4 netCol = network->IsConnected() ? XMFLOAT4(0.25f, 0.95f, 0.45f, 0.95f) : XMFLOAT4(1.0f, 0.85f, 0.25f, 0.95f);
            AddStringLines(netStr, -totalW * 0.5f, 0.26f, tw, th, sp, netCol, dynRetVerts);
        }

        // Player STAGGER OVERLOAD Warning in VR Canopy
        if (mech.IsStaggered())
        {
            static float s_vrStgBlink = 0.0f;
            s_vrStgBlink += 0.016f;
            float stgAlpha = (std::sin(s_vrStgBlink * 20.0f) > 0.0f) ? 1.0f : 0.35f;
            std::string stgWarn = "! STAGGER OVERLOAD !";
            float tw = 0.008f * invAspect;
            float th = 0.016f;
            float sp = 0.0035f * invAspect;
            float totalW = static_cast<float>(stgWarn.length()) * (tw + sp);
            AddStringLines(stgWarn, -totalW * 0.5f, 0.16f, tw, th, sp, XMFLOAT4(1.0f, 0.15f, 0.10f, stgAlpha), dynRetVerts);
        }

        // Boost Kick / Assault Boost Guide in VR Canopy
        if (mech.IsBoostKicking())
        {
            static float s_vrKickBlink = 0.0f;
            s_vrKickBlink += 0.016f;
            float kickAlpha = (std::sin(s_vrKickBlink * 30.0f) > 0.0f) ? 1.0f : 0.4f;
            std::string kickText = ">> BOOST KICK <<";
            float tw = 0.008f * invAspect;
            float th = 0.016f;
            float sp = 0.0035f * invAspect;
            float totalW = static_cast<float>(kickText.length()) * (tw + sp);
            AddStringLines(kickText, -totalW * 0.5f, -0.06f, tw, th, sp, XMFLOAT4(1.0f, 0.40f, 0.10f, kickAlpha), dynRetVerts);
        }
        else if (mech.IsAssaultBoost())
        {
            std::string abGuide = "[L3 / CTRL] KICK   [S] CANCEL";
            float tw = 0.0055f * invAspect;
            float th = 0.011f;
            float sp = 0.0025f * invAspect;
            float totalW = static_cast<float>(abGuide.length()) * (tw + sp);
            AddStringLines(abGuide, -totalW * 0.5f, -0.22f, tw, th, sp, XMFLOAT4(1.0f, 0.75f, 0.20f, 0.95f), dynRetVerts);
        }

        // Damage Direction Indicators in VR Canopy HUD
        const auto& dmgInds = mech.GetDamageIndicators();
        for (const auto& ind : dmgInds)
        {
            if (ind.intensity <= 0.001f) continue;

            float baseRadius = 0.16f;
            float centerPhi = XM_PIDIV2 - ind.relativeAngle;
            float span = XM_PI / 4.0f;
            int segCount = 8;
            float alpha = std::clamp(ind.intensity, 0.0f, 1.0f);
            XMFLOAT4 arcCol = { 1.0f, 0.18f, 0.15f, alpha * 0.95f };

            for (int s = 0; s < segCount; ++s)
            {
                float t1 = -0.5f + static_cast<float>(s) / segCount;
                float t2 = -0.5f + static_cast<float>(s + 1) / segCount;
                float a1 = centerPhi + t1 * span;
                float a2 = centerPhi + t2 * span;

                float x1 = std::cos(a1) * baseRadius;
                float y1 = std::sin(a1) * baseRadius;
                float x2 = std::cos(a2) * baseRadius;
                float y2 = std::sin(a2) * baseRadius;

                dynRetVerts.push_back({ { x1, y1, 0.0f }, { 0, 0, 1 }, arcCol });
                dynRetVerts.push_back({ { x2, y2, 0.0f }, { 0, 0, 1 }, arcCol });
            }

            // Pointer tip
            float tipX = std::cos(centerPhi) * (baseRadius * 1.12f);
            float tipY = std::sin(centerPhi) * (baseRadius * 1.12f);
            float s1X  = std::cos(centerPhi - span * 0.20f) * (baseRadius * 0.94f);
            float s1Y  = std::sin(centerPhi - span * 0.20f) * (baseRadius * 0.94f);
            float s2X  = std::cos(centerPhi + span * 0.20f) * (baseRadius * 0.94f);
            float s2Y  = std::sin(centerPhi + span * 0.20f) * (baseRadius * 0.94f);

            dynRetVerts.push_back({ { tipX, tipY, 0.0f }, { 0, 0, 1 }, arcCol });
            dynRetVerts.push_back({ { s1X, s1Y, 0.0f }, { 0, 0, 1 }, arcCol });
            dynRetVerts.push_back({ { tipX, tipY, 0.0f }, { 0, 0, 1 }, arcCol });
            dynRetVerts.push_back({ { s2X, s2Y, 0.0f }, { 0, 0, 1 }, arcCol });
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

        // 3. Draw EN Bar & Ammo / AP Bars in VR Cockpit
        float enRatio = mech.GetEnergyRatio();
        float barWidth = 0.28f;
        float barHeight = 0.015f;
        float barX = 0.0f;
        float barY = -0.32f;

        float left = barX - barWidth * 0.5f;
        float top = barY + barHeight * 0.5f;
        float bottom = barY - barHeight * 0.5f;
        float currentRight = left + barWidth * enRatio;

        XMFLOAT4 enColor = { 0.1f, 0.8f, 1.0f, 0.9f };
        if (enRatio < 0.25f) enColor = { 1.0f, 0.2f, 0.1f, 0.9f };

        std::vector<Vertex> enVerts = {
            { { left, bottom, 0.0f }, { 0, 0, 1 }, enColor },
            { { left, top, 0.0f }, { 0, 0, 1 }, enColor },
            { { currentRight, top, 0.0f }, { 0, 0, 1 }, enColor },
            { { left, bottom, 0.0f }, { 0, 0, 1 }, enColor },
            { { currentRight, top, 0.0f }, { 0, 0, 1 }, enColor },
            { { currentRight, bottom, 0.0f }, { 0, 0, 1 }, enColor },
        };

        if (SUCCEEDED(m_context->Map(m_dynamicEnBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, enVerts.data(), sizeof(Vertex) * enVerts.size());
            m_context->Unmap(m_dynamicEnBuffer.Get(), 0);
        }
        m_context->IASetVertexBuffers(0, 1, m_dynamicEnBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->Draw(static_cast<UINT>(enVerts.size()), 0);

        // Ammo Bars & AP Bar
        // 4. Ammo Bars (4 Slots: RA, RB, LA, LB) & AP Bar in VR Cockpit
        const auto& sRA = weapons.GetSlot(WeaponSlot::RightArm);
        const auto& sRB = weapons.GetSlot(WeaponSlot::RightBack);
        const auto& sLA = weapons.GetSlot(WeaponSlot::LeftArm);
        const auto& sLB = weapons.GetSlot(WeaponSlot::LeftBack);

        float raRatio = sRA.GetAmmoRatio();
        float rbRatio = sRB.GetAmmoRatio();
        float laRatio = sLA.GetAmmoRatio();
        float lbRatio = sLB.GetAmmoRatio();

        float aLeft = -0.36f;
        float aWidth = 0.14f;
        float aHeight = 0.008f;

        float raTop = -0.355f, raBot = raTop - aHeight;
        float rbTop = -0.370f, rbBot = rbTop - aHeight;
        float laTop = -0.395f, laBot = laTop - aHeight;
        float lbTop = -0.410f, lbBot = lbTop - aHeight;

        float raFill = aLeft + aWidth * raRatio;
        float rbFill = aLeft + aWidth * rbRatio;
        float laFill = aLeft + aWidth * laRatio;
        float lbFill = aLeft + aWidth * lbRatio;

        static float s_vrPulseTime = 0.0f;
        s_vrPulseTime += 0.016f;
        float vrPulse = 0.60f + 0.40f * std::sin(s_vrPulseTime * 14.0f);

        XMFLOAT4 raColor = sRA.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.15f, vrPulse) : XMFLOAT4(1.0f, 0.60f, 0.20f, 0.95f);
        XMFLOAT4 rbColor = sRB.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.15f, vrPulse) : XMFLOAT4(1.0f, 0.80f, 0.22f, 0.90f);
        XMFLOAT4 laColor = sLA.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.15f, vrPulse) : XMFLOAT4(0.25f, 0.85f, 1.00f, 0.95f);
        XMFLOAT4 lbColor = sLB.isReloading ? XMFLOAT4(1.0f, 0.85f, 0.15f, vrPulse) : XMFLOAT4(0.88f, 0.25f, 1.00f, 0.90f);
        XMFLOAT4 vrFrame = { 0.15f, 0.18f, 0.22f, 0.60f };

        // AP Bar
        float apRatio = mech.GetHpRatio();
        float apLeft = 0.22f;
        float apWidth = 0.14f;
        float apTop = -0.355f;
        float apBot = apTop - 0.010f;
        float apFill = apLeft + apWidth * std::clamp(apRatio, 0.0f, 1.0f);
        XMFLOAT4 apColor = (apRatio > 0.35f) ? XMFLOAT4(0.25f, 0.95f, 0.40f, 0.95f) : XMFLOAT4(1.0f, 0.25f, 0.20f, 0.95f);

        // ACS Bar (VR Cockpit)
        float acsRatio = mech.GetAcsRatio();
        float acsTop = apBot - 0.006f;
        float acsBot = acsTop - 0.008f;
        float acsFill = apLeft + apWidth * std::clamp(acsRatio, 0.0f, 1.0f);
        XMFLOAT4 acsColor = mech.IsStaggered() ? XMFLOAT4(1.0f, 0.15f, 0.10f, vrPulse) : (acsRatio > 0.70f ? XMFLOAT4(1.0f, 0.50f, 0.15f, 0.95f) : XMFLOAT4(1.0f, 0.80f, 0.25f, 0.90f));

        std::vector<Vertex> ammoVerts;
        ammoVerts.reserve(96);

        auto addVrBarQuad = [&](float l, float r, float t, float b, XMFLOAT4 col) {
            ammoVerts.push_back({ { l, b, 0.0f }, { 0, 0, 1 }, col });
            ammoVerts.push_back({ { l, t, 0.0f }, { 0, 0, 1 }, col });
            ammoVerts.push_back({ { r, t, 0.0f }, { 0, 0, 1 }, col });
            ammoVerts.push_back({ { l, b, 0.0f }, { 0, 0, 1 }, col });
            ammoVerts.push_back({ { r, t, 0.0f }, { 0, 0, 1 }, col });
            ammoVerts.push_back({ { r, b, 0.0f }, { 0, 0, 1 }, col });
        };

        // RA
        addVrBarQuad(aLeft, aLeft + aWidth, raTop, raBot, vrFrame);
        addVrBarQuad(aLeft, raFill, raTop, raBot, raColor);

        // RB
        addVrBarQuad(aLeft, aLeft + aWidth, rbTop, rbBot, vrFrame);
        addVrBarQuad(aLeft, rbFill, rbTop, rbBot, rbColor);

        // LA
        addVrBarQuad(aLeft, aLeft + aWidth, laTop, laBot, vrFrame);
        addVrBarQuad(aLeft, laFill, laTop, laBot, laColor);

        // LB
        addVrBarQuad(aLeft, aLeft + aWidth, lbTop, lbBot, vrFrame);
        addVrBarQuad(aLeft, lbFill, lbTop, lbBot, lbColor);

        // AP
        addVrBarQuad(apLeft, apLeft + apWidth, apTop, apBot, vrFrame);
        addVrBarQuad(apLeft, apFill, apTop, apBot, apColor);

        // ACS
        addVrBarQuad(apLeft, apLeft + apWidth, acsTop, acsBot, vrFrame);
        addVrBarQuad(apLeft, acsFill, acsTop, acsBot, acsColor);

        if (SUCCEEDED(m_context->Map(m_dynamicAmmoBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            UINT copyCount = static_cast<UINT>(std::min(ammoVerts.size(), size_t(256)));
            memcpy(mapped.pData, ammoVerts.data(), sizeof(Vertex) * copyCount);
            m_context->Unmap(m_dynamicAmmoBuffer.Get(), 0);
        }
        m_context->IASetVertexBuffers(0, 1, m_dynamicAmmoBuffer.GetAddressOf(), &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_context->Draw(static_cast<UINT>(ammoVerts.size()), 0);
    }

    void D3D11Renderer::RenderMainMenu(
        const MenuSystem& menu,
        const Camera& camera,
        const MechController& mech
    )
    {
        float aspect = static_cast<float>(m_width) / static_cast<float>(m_height > 0 ? m_height : 1);
        float invAspect = 1.0f / aspect;
        XMMATRIX view = camera.GetViewMatrix();
        XMMATRIX proj = camera.GetProjectionMatrix(aspect);

        // 1. Render 3D Background: Cyber Grid Floor and Idle Mech
        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = XMMatrixIdentity();
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }
        m_gridFloor.RenderFloorOnly(m_context.Get());
        static WeaponSystem s_menuDefaultWeapons;
        RenderMech(mech, s_menuDefaultWeapons, nullptr, view, proj, false); // Always render full mech in third-person in main menu

        // 2. Render 2D Cyber UI Menu Overlay
        m_context->OMSetDepthStencilState(m_hudDepthDisabledState.Get(), 0);

        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = XMMatrixIdentity();
            cb->view = XMMatrixIdentity();
            cb->projection = XMMatrixIdentity();
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        std::vector<Vertex> uiVerts;
        uiVerts.reserve(4096);
        float animTime = menu.GetAnimTime();

        // Cyber Palette
        XMFLOAT4 cyan      = { 0.25f, 0.90f, 1.00f, 0.95f };
        XMFLOAT4 dimCyan   = { 0.15f, 0.45f, 0.60f, 0.70f };
        XMFLOAT4 amber     = { 1.00f, 0.78f, 0.18f, 0.95f };
        XMFLOAT4 white     = { 0.95f, 0.96f, 1.00f, 0.95f };
        XMFLOAT4 dimText   = { 0.60f, 0.68f, 0.75f, 0.85f };

        float pulse = 0.75f + 0.25f * std::sin(animTime * 6.5f);
        XMFLOAT4 activeCol = { 1.00f, 0.82f, 0.18f, pulse };

        auto addLine = [&](float x1, float y1, float x2, float y2, XMFLOAT4 col) {
            uiVerts.push_back({ { x1, y1, 0.0f }, { 0, 0, 1 }, col });
            uiVerts.push_back({ { x2, y2, 0.0f }, { 0, 0, 1 }, col });
        };

        auto addRect = [&](float l, float t, float r, float b, XMFLOAT4 col) {
            addLine(l, t, r, t, col);
            addLine(r, t, r, b, col);
            addLine(r, b, l, b, col);
            addLine(l, b, l, t, col);
        };

        // (A) Top Banner & Title Box
        float tBoxL = -0.72f, tBoxR = 0.72f;
        float tBoxT = 0.88f,  tBoxB = 0.68f;
        float cLen = 0.045f;
        addLine(tBoxL, tBoxT, tBoxL + cLen, tBoxT, cyan);
        addLine(tBoxL, tBoxT, tBoxL, tBoxT - cLen, cyan);
        addLine(tBoxR, tBoxT, tBoxR - cLen, tBoxT, cyan);
        addLine(tBoxR, tBoxT, tBoxR, tBoxT - cLen, cyan);
        addLine(tBoxL, tBoxB, tBoxL + cLen, tBoxB, cyan);
        addLine(tBoxL, tBoxB, tBoxL, tBoxB + cLen, cyan);
        addLine(tBoxR, tBoxB, tBoxR - cLen, tBoxB, cyan);
        addLine(tBoxR, tBoxB, tBoxR, tBoxB + cLen, cyan);

        // Title text: "OVERDRIVE CORE"
        std::string mainTitle = "OVERDRIVE CORE";
        float tCharH = 0.065f;
        float tCharW = tCharH * invAspect * 0.75f;
        float tSp = tCharW * 0.40f;
        float tTotalW = mainTitle.length() * (tCharW + tSp);
        AddStringLines(mainTitle, -tTotalW * 0.5f, 0.79f, tCharW, tCharH, tSp, white, uiVerts);

        // Subtitle: "TACTICAL HIGH-SPEED MECH SIMULATOR"
        std::string subTitle = "TACTICAL HIGH-SPEED MECH SIMULATOR";
        float sCharH = 0.022f;
        float sCharW = sCharH * invAspect * 0.70f;
        float sSp = sCharW * 0.35f;
        float sTotalW = subTitle.length() * (sCharW + sSp);
        AddStringLines(subTitle, -sTotalW * 0.5f, 0.715f, sCharW, sCharH, sSp, cyan, uiVerts);

        // (B) Menu Item List
        const auto& items = menu.GetItems();
        int selectedIdx = menu.GetSelectedIndex();
        float startY = 0.38f;
        float itemHeight = 0.12f;
        float mCharH = 0.038f;
        float mCharW = mCharH * invAspect * 0.75f;
        float mSp = mCharW * 0.35f;

        float menuBoxW = 0.85f;
        float menuBoxL = -menuBoxW * 0.5f;
        float menuBoxR =  menuBoxW * 0.5f;

        for (size_t i = 0; i < items.size(); ++i)
        {
            float itemY = startY - i * itemHeight;
            bool isSel = (static_cast<int>(i) == selectedIdx);

            if (isSel)
            {
                float pad = 0.025f;
                float topY = itemY + mCharH * 0.5f + pad;
                float botY = itemY - mCharH * 0.5f - pad;
                addRect(menuBoxL, topY, menuBoxR, botY, activeCol);

                // Cursor: "> "
                std::string cur = "> ";
                AddStringLines(cur, menuBoxL + 0.03f, itemY, mCharW, mCharH, mSp, activeCol, uiVerts);
            }
            else
            {
                float botY = itemY - mCharH * 0.5f - 0.02f;
                addLine(menuBoxL, botY, menuBoxR, botY, dimCyan);
            }

            XMFLOAT4 itemCol = isSel ? white : (items[i].isEnabled ? dimText : XMFLOAT4(0.4f, 0.4f, 0.4f, 0.5f));
            float textX = menuBoxL + 0.10f;
            AddStringLines(items[i].label, textX, itemY, mCharW, mCharH, mSp, itemCol, uiVerts);
        }

        // (C) Description Box
        const MenuItem* curItem = menu.GetSelectedItem();
        if (curItem)
        {
            float descY = -0.32f;
            float dBoxL = -0.75f, dBoxR = 0.75f;
            float dBoxT = descY + 0.055f, dBoxB = descY - 0.055f;

            addLine(dBoxL, dBoxT, dBoxR, dBoxT, dimCyan);
            addLine(dBoxL, dBoxB, dBoxR, dBoxB, dimCyan);

            std::string fullDesc = "INFO: " + curItem->description;
            float dCharH = 0.021f;
            float dCharW = dCharH * invAspect * 0.70f;
            float dSp = dCharW * 0.35f;
            float dTotalW = fullDesc.length() * (dCharW + dSp);
            AddStringLines(fullDesc, -dTotalW * 0.5f, descY, dCharW, dCharH, dSp, amber, uiVerts);
        }

        // (D) Footer Controls Guide
        std::string footer = "[W/S/UP/DOWN] SELECT    [ENTER/SPACE/A] CONFIRM    [ESC] BACK";
        float fCharH = 0.020f;
        float fCharW = fCharH * invAspect * 0.70f;
        float fSp = fCharW * 0.35f;
        float fTotalW = footer.length() * (fCharW + fSp);
        AddStringLines(footer, -fTotalW * 0.5f, -0.78f, fCharW, fCharH, fSp, cyan, uiVerts);

        // Upload and draw UI
        if (!uiVerts.empty())
        {
            D3D11_MAPPED_SUBRESOURCE dynMap;
            if (SUCCEEDED(m_context->Map(m_dynamicReticleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dynMap)))
            {
                UINT copyCount = static_cast<UINT>(std::min(uiVerts.size(), size_t(4096)));
                memcpy(dynMap.pData, uiVerts.data(), sizeof(Vertex) * copyCount);
                m_context->Unmap(m_dynamicReticleBuffer.Get(), 0);

                UINT stride = sizeof(Vertex);
                UINT offset = 0;
                m_context->IASetVertexBuffers(0, 1, m_dynamicReticleBuffer.GetAddressOf(), &stride, &offset);
                m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                m_context->Draw(copyCount, 0);
            }
        }

        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 1);
    }

    void D3D11Renderer::RenderDirectConnectMenu(
        const Camera& camera,
        const MechController& mech,
        const std::string& inputIp,
        const std::string& myTailscaleIp,
        uint16_t port,
        float animTime,
        const std::string& notificationMessage
    )
    {
        float aspect = static_cast<float>(m_width) / static_cast<float>(m_height > 0 ? m_height : 1);
        float invAspect = 1.0f / aspect;
        XMMATRIX view = camera.GetViewMatrix();
        XMMATRIX proj = camera.GetProjectionMatrix(aspect);

        // 1. Render 3D Background: Cyber Grid Floor and Mech
        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = XMMatrixIdentity();
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }
        m_gridFloor.RenderFloorOnly(m_context.Get());
        static WeaponSystem s_defaultWeapons;
        RenderMech(mech, s_defaultWeapons, nullptr, view, proj, false);

        // 2. Render 2D Cyber UI
        m_context->OMSetDepthStencilState(m_hudDepthDisabledState.Get(), 0);

        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = XMMatrixIdentity();
            cb->view = XMMatrixIdentity();
            cb->projection = XMMatrixIdentity();
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        std::vector<Vertex> uiVerts;
        uiVerts.reserve(4096);

        XMFLOAT4 cyan      = { 0.25f, 0.90f, 1.00f, 0.95f };
        XMFLOAT4 dimCyan   = { 0.15f, 0.45f, 0.60f, 0.70f };
        XMFLOAT4 amber     = { 1.00f, 0.78f, 0.18f, 0.95f };
        XMFLOAT4 white     = { 0.95f, 0.96f, 1.00f, 0.95f };
        XMFLOAT4 dimText   = { 0.60f, 0.68f, 0.75f, 0.85f };
        XMFLOAT4 green     = { 0.20f, 0.95f, 0.45f, 0.95f };

        float pulse = 0.75f + 0.25f * std::sin(animTime * 6.5f);
        XMFLOAT4 activeCol = { 1.00f, 0.82f, 0.18f, pulse };

        auto addLine = [&](float x1, float y1, float x2, float y2, XMFLOAT4 col) {
            uiVerts.push_back({ { x1, y1, 0.0f }, { 0, 0, 1 }, col });
            uiVerts.push_back({ { x2, y2, 0.0f }, { 0, 0, 1 }, col });
        };

        auto addRect = [&](float l, float t, float r, float b, XMFLOAT4 col) {
            addLine(l, t, r, t, col);
            addLine(r, t, r, b, col);
            addLine(r, b, l, b, col);
            addLine(l, b, l, t, col);
        };

        // (A) Top Banner & Title Box
        float tBoxL = -0.72f, tBoxR = 0.72f;
        float tBoxT = 0.88f,  tBoxB = 0.68f;
        float cLen = 0.045f;
        addLine(tBoxL, tBoxT, tBoxL + cLen, tBoxT, cyan);
        addLine(tBoxL, tBoxT, tBoxL, tBoxT - cLen, cyan);
        addLine(tBoxR, tBoxT, tBoxR - cLen, tBoxT, cyan);
        addLine(tBoxR, tBoxT, tBoxR, tBoxT - cLen, cyan);
        addLine(tBoxL, tBoxB, tBoxL + cLen, tBoxB, cyan);
        addLine(tBoxL, tBoxB, tBoxL, tBoxB + cLen, cyan);
        addLine(tBoxR, tBoxB, tBoxR - cLen, tBoxB, cyan);
        addLine(tBoxR, tBoxB, tBoxR, tBoxB + cLen, cyan);

        std::string title = "DIRECT IP CONNECT";
        float tCharH = 0.060f;
        float tCharW = tCharH * invAspect * 0.75f;
        float tSp = tCharW * 0.40f;
        float tTotalW = title.length() * (tCharW + tSp);
        AddStringLines(title, -tTotalW * 0.5f, 0.79f, tCharW, tCharH, tSp, white, uiVerts);

        std::string sub = "TAILSCALE / VPN / LOCAL LAN MULTIPLAYER";
        float sCharH = 0.022f;
        float sCharW = sCharH * invAspect * 0.70f;
        float sSp = sCharW * 0.35f;
        float sTotalW = sub.length() * (sCharW + sSp);
        AddStringLines(sub, -sTotalW * 0.5f, 0.715f, sCharW, sCharH, sSp, cyan, uiVerts);

        // (B) Main Center Box: Target Host IP
        float mBoxL = -0.65f, mBoxR = 0.65f;
        float mBoxT = 0.48f,  mBoxB = 0.05f;
        addRect(mBoxL, mBoxT, mBoxR, mBoxB, dimCyan);

        std::string prompt = "ENTER HOST IP ADDRESS (PORT: " + std::to_string(port) + "):";
        float pCharH = 0.024f;
        float pCharW = pCharH * invAspect * 0.70f;
        float pSp = pCharW * 0.35f;
        AddStringLines(prompt, mBoxL + 0.05f, 0.40f, pCharW, pCharH, pSp, cyan, uiVerts);

        // IP Input Field Box
        float ipBoxL = mBoxL + 0.05f, ipBoxR = mBoxR - 0.05f;
        float ipBoxT = 0.32f,         ipBoxB = 0.17f;
        addRect(ipBoxL, ipBoxT, ipBoxR, ipBoxB, activeCol);

        // Blinking cursor
        std::string displayIp = inputIp;
        if (fmod(animTime, 0.8f) < 0.4f)
        {
            displayIp += "_";
        }
        else
        {
            displayIp += " ";
        }

        float ipCharH = 0.052f;
        float ipCharW = ipCharH * invAspect * 0.75f;
        float ipSp = ipCharW * 0.35f;
        AddStringLines(displayIp, ipBoxL + 0.04f, 0.245f, ipCharW, ipCharH, ipSp, white, uiVerts);

        // Quick Paste & Type Instructions
        std::string pasteGuide = "[CTRL+V] PASTE FROM CLIPBOARD    [0-9 / .] TYPE    [BACKSPACE] DELETE";
        float gCharH = 0.019f;
        float gCharW = gCharH * invAspect * 0.70f;
        float gSp = gCharW * 0.35f;
        AddStringLines(pasteGuide, mBoxL + 0.05f, 0.10f, gCharW, gCharH, gSp, amber, uiVerts);

        // (C) Host / Self Info Box
        float hBoxL = -0.65f, hBoxR = 0.65f;
        float hBoxT = -0.05f, hBoxB = -0.35f;
        addRect(hBoxL, hBoxT, hBoxR, hBoxB, dimCyan);

        std::string myIpTitle = "YOUR DETECTED IP (TAILSCALE / LAN):";
        AddStringLines(myIpTitle, hBoxL + 0.05f, -0.12f, pCharH, pCharH * invAspect * 0.70f, pSp, cyan, uiVerts);

        std::string myIpStr = myTailscaleIp + ":" + std::to_string(port);
        AddStringLines(myIpStr, hBoxL + 0.05f, -0.20f, 0.038f, 0.038f * invAspect * 0.75f, pSp, green, uiVerts);

        std::string shareHint = "IF YOU HOST, SHARE THIS IP WITH FRIENDS - PRESS [C] TO COPY YOUR IP";
        AddStringLines(shareHint, hBoxL + 0.05f, -0.29f, 0.019f, 0.019f * invAspect * 0.70f, pSp, dimText, uiVerts);

        // (D) Notification Message Banner (if any)
        if (!notificationMessage.empty())
        {
            float nCharH = 0.024f;
            float nCharW = nCharH * invAspect * 0.70f;
            float nSp = nCharW * 0.35f;
            float nTotalW = notificationMessage.length() * (nCharW + nSp);
            AddStringLines(notificationMessage, -nTotalW * 0.5f, -0.45f, nCharW, nCharH, nSp, activeCol, uiVerts);
        }

        // (E) Footer Navigation Guide
        std::string footer = "[ENTER / A] CONNECT TO BATTLE    [ESC / B] CANCEL & RETURN";
        float fCharH = 0.024f;
        float fCharW = fCharH * invAspect * 0.70f;
        float fSp = fCharW * 0.35f;
        float fTotalW = footer.length() * (fCharW + fSp);
        AddStringLines(footer, -fTotalW * 0.5f, -0.75f, fCharW, fCharH, fSp, cyan, uiVerts);

        // Upload and draw UI
        if (!uiVerts.empty())
        {
            D3D11_MAPPED_SUBRESOURCE dynMap;
            if (SUCCEEDED(m_context->Map(m_dynamicReticleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dynMap)))
            {
                UINT copyCount = static_cast<UINT>(std::min(uiVerts.size(), size_t(4096)));
                memcpy(dynMap.pData, uiVerts.data(), sizeof(Vertex) * copyCount);
                m_context->Unmap(m_dynamicReticleBuffer.Get(), 0);

                UINT stride = sizeof(Vertex);
                UINT offset = 0;
                m_context->IASetVertexBuffers(0, 1, m_dynamicReticleBuffer.GetAddressOf(), &stride, &offset);
                m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                m_context->Draw(copyCount, 0);
            }
        }

        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 1);
    }

    void D3D11Renderer::RenderAssembleMenu(
        const Camera& camera,
        const MechController& mech,
        const WeaponSystem& weapons,
        const FrameSystem& frames,
        const InternalSystem& internal,
        AssembleTab currentTab,
        int selectedSlotIndex,
        float animTime
    )
    {
        float aspect = static_cast<float>(m_width) / static_cast<float>(m_height > 0 ? m_height : 1);
        float invAspect = 1.0f / aspect;
        XMMATRIX view = camera.GetViewMatrix();
        XMMATRIX proj = camera.GetProjectionMatrix(aspect);

        // 1. Render 3D Background: Cyber Grid Floor and Mech with current custom loadout & frame
        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = XMMatrixIdentity();
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }
        m_gridFloor.RenderFloorOnly(m_context.Get());
        // Render mech with the actual weapons and frame currently equipped!
        RenderMech(mech, weapons, &frames, view, proj, false);

        // 2. Render 2D Cyber Assemble UI Overlay
        m_context->OMSetDepthStencilState(m_hudDepthDisabledState.Get(), 0);

        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = XMMatrixIdentity();
            cb->view = XMMatrixIdentity();
            cb->projection = XMMatrixIdentity();
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        std::vector<Vertex> uiVerts;
        uiVerts.reserve(4096);

        XMFLOAT4 cyan      = { 0.25f, 0.90f, 1.00f, 0.95f };
        XMFLOAT4 dimCyan   = { 0.15f, 0.45f, 0.60f, 0.70f };
        XMFLOAT4 amber     = { 1.00f, 0.82f, 0.18f, 0.95f };
        XMFLOAT4 white     = { 0.95f, 0.96f, 1.00f, 0.95f };
        XMFLOAT4 dimText   = { 0.60f, 0.68f, 0.75f, 0.85f };
        XMFLOAT4 green     = { 0.20f, 0.95f, 0.50f, 0.95f };
        XMFLOAT4 orange    = { 1.00f, 0.55f, 0.15f, 0.95f };

        float pulse = 0.75f + 0.25f * std::sin(animTime * 7.0f);
        XMFLOAT4 activeAmber = { 1.00f, 0.82f, 0.18f, pulse };
        XMFLOAT4 activeCyan  = { 0.25f, 0.90f, 1.00f, pulse };

        auto addLine = [&](float x1, float y1, float x2, float y2, XMFLOAT4 col) {
            uiVerts.push_back({ { x1, y1, 0.0f }, { 0, 0, 1 }, col });
            uiVerts.push_back({ { x2, y2, 0.0f }, { 0, 0, 1 }, col });
        };

        auto addRect = [&](float l, float t, float r, float b, XMFLOAT4 col) {
            addLine(l, t, r, t, col);
            addLine(r, t, r, b, col);
            addLine(r, b, l, b, col);
            addLine(l, b, l, t, col);
        };

        // (A) Top Banner & Title Box
        float tBoxL = -0.92f, tBoxR = 0.92f;
        float tBoxT = 0.90f,  tBoxB = 0.75f;
        float cLen = 0.035f;
        addLine(tBoxL, tBoxT, tBoxL + cLen, tBoxT, cyan);
        addLine(tBoxL, tBoxT, tBoxL, tBoxT - cLen, cyan);
        addLine(tBoxR, tBoxT, tBoxR - cLen, tBoxT, cyan);
        addLine(tBoxR, tBoxT, tBoxR, tBoxT - cLen, cyan);
        addLine(tBoxL, tBoxB, tBoxL + cLen, tBoxB, cyan);
        addLine(tBoxL, tBoxB, tBoxL, tBoxB + cLen, cyan);
        addLine(tBoxR, tBoxB, tBoxR - cLen, tBoxB, cyan);
        addLine(tBoxR, tBoxB, tBoxR, tBoxB + cLen, cyan);

        std::string mainTitle = "GARAGE // AC ASSEMBLE SYSTEM";
        float tCharH = 0.040f;
        float tCharW = tCharH * invAspect * 0.75f;
        float tSp = tCharW * 0.35f;
        AddStringLines(mainTitle, -0.88f, 0.845f, tCharW, tCharH, tSp, white, uiVerts);

        // 3-Tab Switchers: [1] WEAPONS | [2] FRAME | [3] INTERNAL
        float tabW = 0.28f;
        float tab1L = -0.06f, tab1R = tab1L + tabW;
        float tab2L = tab1R + 0.03f, tab2R = tab2L + tabW;
        float tab3L = tab2R + 0.03f, tab3R = tab3L + tabW;
        float tabT = 0.875f,  tabB = 0.775f;

        bool isWeaponsTab  = (currentTab == AssembleTab::Weapons);
        bool isFrameTab    = (currentTab == AssembleTab::Frame);
        bool isInternalTab = (currentTab == AssembleTab::Internal);

        addRect(tab1L, tabT, tab1R, tabB, isWeaponsTab ? activeAmber : dimCyan);
        addRect(tab2L, tabT, tab2R, tabB, isFrameTab ? activeAmber : dimCyan);
        addRect(tab3L, tabT, tab3R, tabB, isInternalTab ? activeAmber : dimCyan);

        float tabCharH = 0.022f;
        float tabCharW = tabCharH * invAspect * 0.72f;
        float tabSp = tabCharW * 0.30f;

        std::string tab1Text = isWeaponsTab  ? "> WEAPONS UNIT <"  : "  WEAPONS UNIT  ";
        std::string tab2Text = isFrameTab    ? "> FRAME PARTS <"   : "  FRAME PARTS  ";
        std::string tab3Text = isInternalTab ? "> INTERNAL PARTS <": "  INTERNAL PARTS  ";
        AddStringLines(tab1Text, tab1L + 0.02f, 0.835f, tabCharW, tabCharH, tabSp, isWeaponsTab ? amber : dimText, uiVerts);
        AddStringLines(tab2Text, tab2L + 0.02f, 0.835f, tabCharW, tabCharH, tabSp, isFrameTab ? amber : dimText, uiVerts);
        AddStringLines(tab3Text, tab3L + 0.02f, 0.835f, tabCharW, tabCharH, tabSp, isInternalTab ? amber : dimText, uiVerts);

        std::string subTitle;
        if (isWeaponsTab)       subTitle = "EQUIP ARMS AND BACK MOUNTED WEAPONS";
        else if (isFrameTab)    subTitle = "CUSTOMIZE HEAD, CORE, ARMS, LEGS FRAMES";
        else                    subTitle = "CUSTOMIZE BOOSTER, FCS CHIP, AND GENERATOR";

        float sCharH = 0.017f;
        float sCharW = sCharH * invAspect * 0.70f;
        float sSp = sCharW * 0.32f;
        AddStringLines(subTitle, -0.88f, 0.785f, sCharW, sCharH, sSp, cyan, uiVerts);

        // (B) Left Panel: Slots Selection
        float panelL = -0.92f;
        float panelW = 0.72f;
        float panelR = panelL + panelW;
        float slotStartY = 0.64f;
        float slotSpacing = 0.17f;

        const char* weaponSlotLabels[4] = {
            "[1] RIGHT ARM UNIT",
            "[2] LEFT ARM UNIT",
            "[3] RIGHT BACK UNIT",
            "[4] LEFT BACK UNIT"
        };

        const char* frameSlotLabels[4] = {
            "[1] HEAD UNIT",
            "[2] CORE UNIT",
            "[3] ARMS UNIT",
            "[4] LEGS UNIT"
        };

        const char* internalSlotLabels[3] = {
            "[1] BOOSTER UNIT",
            "[2] FCS CHIP UNIT",
            "[3] GENERATOR UNIT"
        };

        int slotCount = isInternalTab ? 3 : 4;

        for (int i = 0; i < slotCount; ++i)
        {
            bool isSelected = (i == selectedSlotIndex);
            float sTop = slotStartY - i * slotSpacing;
            float sBot = sTop - 0.13f;

            XMFLOAT4 frameCol = isSelected ? activeAmber : dimCyan;
            addRect(panelL, sTop, panelR, sBot, frameCol);

            // Corner decorative notches on selected slot
            if (isSelected)
            {
                float nLen = 0.025f;
                addLine(panelL, sTop, panelL, sTop - nLen, amber);
                addLine(panelR, sTop, panelR, sTop - nLen, amber);
                addLine(panelL, sBot, panelL, sBot + nLen, amber);
                addLine(panelR, sBot, panelR, sBot + nLen, amber);
            }

            // Slot Title
            float slH = 0.024f;
            float slW = slH * invAspect * 0.72f;
            float slSp = slW * 0.32f;
            std::string slotHeader;
            if (isWeaponsTab)       slotHeader = weaponSlotLabels[i];
            else if (isFrameTab)    slotHeader = frameSlotLabels[i];
            else                    slotHeader = internalSlotLabels[i];

            slotHeader = (isSelected ? "> " : "  ") + slotHeader;
            AddStringLines(slotHeader, panelL + 0.02f, sTop - 0.038f, slW, slH, slSp, isSelected ? amber : cyan, uiVerts);

            std::string itemName = "EMPTY SLOT";
            std::string itemTag = "";

            if (isWeaponsTab)
            {
                WeaponSlot wSlot = static_cast<WeaponSlot>(i);
                const auto& instance = weapons.GetSlot(wSlot);
                if (instance.IsValid())
                {
                    itemName = instance.data.name;
                    if (instance.data.category == WeaponCategory::BackOnlyWeapon)
                    {
                        itemTag = "[BACK MISSILE POD]";
                    }
                    else
                    {
                        itemTag = (i >= 2) ? "[HANGER ARM WEAPON]" : "[ARM WEAPON]";
                    }
                }
            }
            else if (isFrameTab)
            {
                FrameSlot fSlot = static_cast<FrameSlot>(i);
                const auto& instance = frames.GetSlot(fSlot);
                if (instance.IsValid())
                {
                    itemName = instance.data.name;
                    const char* slotCatNames[4] = { "HEAD", "CORE", "ARMS", "LEGS" };
                    itemTag = std::string("[") + slotCatNames[i] + " FRAME]";
                }
            }
            else // Internal Tab
            {
                if (i == 0)
                {
                    const auto& bst = internal.GetCurrentBooster();
                    itemName = bst.name;
                    itemTag = "[MAIN THRUSTER UNIT]";
                }
                else if (i == 1)
                {
                    const auto& fcs = internal.GetCurrentFcs();
                    itemName = fcs.name;
                    itemTag = "[FIRE CONTROL SYSTEM]";
                }
                else if (i == 2)
                {
                    const auto& gen = internal.GetCurrentGenerator();
                    itemName = gen.name;
                    itemTag = "[ENERGY GENERATOR]";
                }
            }

            float wH = 0.026f;
            float wW = wH * invAspect * 0.75f;
            float wSp = wW * 0.32f;
            AddStringLines(itemName, panelL + 0.04f, sTop - 0.078f, wW, wH, wSp, isSelected ? white : dimText, uiVerts);

            float tgH = 0.018f;
            float tgW = tgH * invAspect * 0.70f;
            float tgSp = tgW * 0.30f;
            AddStringLines(itemTag, panelL + 0.04f, sTop - 0.110f, tgW, tgH, tgSp, isSelected ? green : dimCyan, uiVerts);

            // Left/Right change arrows if selected
            if (isSelected)
            {
                std::string changeHint = "< [A]   [D] >";
                float chH = 0.020f;
                float chW = chH * invAspect * 0.70f;
                float chSp = chW * 0.30f;
                AddStringLines(changeHint, panelR - 0.16f, sTop - 0.110f, chW, chH, chSp, amber, uiVerts);
            }
        }

        // (C) Right Panel: Selected Part Detailed Parameters & Specs
        float rPanelL = 0.32f;
        float rPanelR = 0.92f;
        float rPanelT = 0.64f;
        float rPanelB = -0.02f;

        addRect(rPanelL, rPanelT, rPanelR, rPanelB, cyan);
        addLine(rPanelL, rPanelT - 0.06f, rPanelR, rPanelT - 0.06f, dimCyan);

        float hdrH = 0.025f;
        float hdrW = hdrH * invAspect * 0.72f;
        float hdrSp = hdrW * 0.32f;
        AddStringLines("PART SPECIFICATIONS", rPanelL + 0.03f, rPanelT - 0.040f, hdrW, hdrH, hdrSp, cyan, uiVerts);

        float paramH = 0.020f;
        float paramW = paramH * invAspect * 0.72f;
        float paramSp = paramW * 0.30f;
        float lineY = rPanelT - 0.095f;
        float lineStep = 0.048f;
        float valX = rPanelL + 0.28f;

        auto drawParam = [&](const std::string& name, const std::string& val, XMFLOAT4 valCol = { 0.95f, 0.96f, 1.00f, 0.95f }) {
            AddStringLines(name, rPanelL + 0.03f, lineY, paramW, paramH, paramSp, dimText, uiVerts);
            AddStringLines(val, valX, lineY, paramW, paramH, paramSp, valCol, uiVerts);
            lineY -= lineStep;
        };

        if (isWeaponsTab)
        {
            WeaponSlot wSlot = static_cast<WeaponSlot>(selectedSlotIndex);
            const auto& curInst = weapons.GetSlot(wSlot);
            if (curInst.IsValid())
            {
                const auto& d = curInst.data;
                drawParam("MODEL ID", d.id, amber);
                drawParam("CATEGORY", (d.category == WeaponCategory::BackOnlyWeapon ? "BACK ONLY (MISSILE)" : "ARM / HANGER WEAPON"), green);
                drawParam("ATTACK POWER", std::to_string(static_cast<int>(d.damage)), white);
                drawParam("IMPACT FORCE", std::to_string(static_cast<int>(d.impact)) + " ACS", orange);
                char dHitBuf[32];
                sprintf_s(dHitBuf, "%.1fx CRIT", d.directHitMult);
                drawParam("DIRECT HIT MULT", dHitBuf, amber);
                drawParam("MAGAZINE CAP", std::to_string(d.maxAmmo) + " ROUNDS", white);

                char rateBuf[32];
                sprintf_s(rateBuf, "%.2f SEC", d.fireRate);
                drawParam("CYCLE TIME", rateBuf, white);

                char rldBuf[32];
                sprintf_s(rldBuf, "%.1f SEC", d.reloadDuration);
                drawParam("RELOAD TIME", rldBuf, white);

                if (d.projectileSpeed > 0.0f)
                {
                    drawParam("MUZZLE VEL", std::to_string(static_cast<int>(d.projectileSpeed)) + " M/S", white);
                }
                else
                {
                    drawParam("MUZZLE VEL", "MELEE CONTACT", green);
                }

                std::string traits;
                if (d.isMelee) traits = "HIGH-FREQ PLASMA CLEAVE";
                else if (d.isHoming) traits = "4-MISSILE VERTICAL HOMING";
                else if (d.pelletCount > 1) traits = "8-PELLET CONICAL SPREAD";
                else if (d.splashRadius > 6.0f) traits = "HEAVY EXPLOSIVE SPLASH";
                else if (d.fireRate <= 0.06f) traits = "20-RPS ROTARY BARRAGE";
                else traits = "PRECISION KINETIC / BEAM";

                drawParam("SPECIAL TRAIT", traits, amber);
            }
        }
        else if (isFrameTab)
        {
            FrameSlot fSlot = static_cast<FrameSlot>(selectedSlotIndex);
            const auto& curInst = frames.GetSlot(fSlot);
            if (curInst.IsValid())
            {
                const auto& d = curInst.data;
                const char* slotCatNames[4] = { "HEAD", "CORE", "ARMS", "LEGS" };
                drawParam("MODEL ID", d.id, amber);
                drawParam("CATEGORY", std::string(slotCatNames[selectedSlotIndex]) + " UNIT", green);

                std::string apStr = std::to_string(static_cast<int>(d.ap)) + " AP";
                drawParam("ARMOR POINTS", apStr, white);

                char acsStabBuf[32];
                sprintf_s(acsStabBuf, "+%d ACS", static_cast<int>(d.attitudeStability));
                drawParam("ATTITUDE STABILITY", acsStabBuf, amber);

                drawParam("WEIGHT", std::to_string(static_cast<int>(d.weight)) + " KG", white);

                char enCostBuf[32];
                sprintf_s(enCostBuf, "%d EN", static_cast<int>(d.energyLoad));
                drawParam("EN DRAIN", enCostBuf, white);

                if (fSlot == FrameSlot::Head)
                {
                    drawParam("SYSTEM RECOVERY", "HIGH (ATTITUDE STABILITY)", green);
                    drawParam("SCAN DISTANCE", "450 M", white);
                }
                else if (fSlot == FrameSlot::Core)
                {
                    drawParam("BOOSTER EFFICIENCY", "OPTIMIZED", green);
                    char capBuf[32];
                    sprintf_s(capBuf, "+%d EN", static_cast<int>(d.energyCapacity));
                    drawParam("GENERATOR POOL", capBuf, amber);
                }
                else if (fSlot == FrameSlot::Arms)
                {
                    char recBuf[32];
                    sprintf_s(recBuf, "%.1fx DAMP", d.recoilControl);
                    drawParam("RECOIL CONTROL", recBuf, green);
                    char aimBuf[32];
                    sprintf_s(aimBuf, "%.1fx SPEED", d.aimSpeed);
                    drawParam("SERVO TRACKING", aimBuf, amber);
                }
                else if (fSlot == FrameSlot::Legs)
                {
                    char spdBuf[32];
                    sprintf_s(spdBuf, "%.2fx", d.speedMod);
                    drawParam("SPEED MULT", spdBuf, green);
                    char jmpBuf[32];
                    sprintf_s(jmpBuf, "%.2fx", d.jumpMod);
                    drawParam("JUMP THRUST", jmpBuf, amber);
                }

                drawParam("FRAME TYPE", d.name, orange);
            }
        }
        else // Internal Tab
        {
            if (selectedSlotIndex == 0) // BOOSTER
            {
                const auto& bst = internal.GetCurrentBooster();
                drawParam("MODEL ID", bst.id, amber);
                drawParam("CATEGORY", "MAIN BOOSTER THRUSTER", green);
                drawParam("CRUISE THRUST", std::to_string(static_cast<int>(bst.thrustSpeed)) + " M/S", white);
                drawParam("QB THRUST", std::to_string(static_cast<int>(bst.qbThrust)) + " M/S", amber);
                drawParam("QB EN DRAIN", std::to_string(static_cast<int>(bst.qbEnergyCost)) + " EN", orange);
                drawParam("VERTICAL LIFT", std::to_string(static_cast<int>(bst.upwardThrust)), cyan);
                drawParam("WEIGHT", std::to_string(static_cast<int>(bst.weight)) + " KG", white);
                drawParam("SPECIAL TRAIT", bst.description, green);
            }
            else if (selectedSlotIndex == 1) // FCS
            {
                const auto& fcs = internal.GetCurrentFcs();
                drawParam("MODEL ID", fcs.id, amber);
                drawParam("CATEGORY", "FIRE CONTROL SYSTEM (FCS)", green);

                char closeBuf[32], midBuf[32], longBuf[32], misBuf[32];
                sprintf_s(closeBuf, "%.1fx (< 130M)", fcs.closeAssist);
                sprintf_s(midBuf,   "%.1fx (130-260M)", fcs.mediumAssist);
                sprintf_s(longBuf,  "%.1fx (> 260M)", fcs.longAssist);
                sprintf_s(misBuf,   "%.2fx TIME", fcs.missileLockMod);

                drawParam("CLOSE ASSIST", closeBuf, fcs.closeAssist >= 1.5f ? green : (fcs.closeAssist <= 0.6f ? orange : white));
                drawParam("MID ASSIST", midBuf, fcs.mediumAssist >= 1.5f ? green : white);
                drawParam("LONG ASSIST", longBuf, fcs.longAssist >= 1.5f ? green : (fcs.longAssist <= 0.6f ? orange : white));
                drawParam("MISSILE LOCK", misBuf, fcs.missileLockMod <= 0.8f ? green : white);
                drawParam("WEIGHT", std::to_string(static_cast<int>(fcs.weight)) + " KG", white);
                drawParam("SPECIAL TRAIT", fcs.description, amber);
            }
            else if (selectedSlotIndex == 2) // GENERATOR
            {
                const auto& gen = internal.GetCurrentGenerator();
                drawParam("MODEL ID", gen.id, amber);
                drawParam("CATEGORY", "INTERNAL GENERATOR", green);
                drawParam("EN CAPACITY", std::to_string(static_cast<int>(gen.energyCapacity)) + " EN", cyan);
                drawParam("RECHARGE RATE", std::to_string(static_cast<int>(gen.rechargeRate)) + " EN/S", green);
                drawParam("AIR RECHARGE", std::to_string(static_cast<int>(gen.rechargeRateAir)) + " EN/S", white);

                char delBuf[32];
                sprintf_s(delBuf, "%.2f SEC", gen.recoveryDelay);
                drawParam("SUPPLY DELAY", delBuf, gen.recoveryDelay <= 0.25f ? green : amber);
                drawParam("WEIGHT", std::to_string(static_cast<int>(gen.weight)) + " KG", white);
                drawParam("SPECIAL TRAIT", gen.description, orange);
            }
        }

        // (C-2) Lower Right Panel: Total AC Performance Summary
        float bPanelL = 0.32f;
        float bPanelR = 0.92f;
        float bPanelT = -0.06f;
        float bPanelB = -0.71f;

        addRect(bPanelL, bPanelT, bPanelR, bPanelB, amber);
        addLine(bPanelL, bPanelT - 0.055f, bPanelR, bPanelT - 0.055f, dimCyan);

        AddStringLines("AC TOTAL PERFORMANCE SUMMARY", bPanelL + 0.03f, bPanelT - 0.038f, hdrW, hdrH, hdrSp, amber, uiVerts);

        float sLineY = bPanelT - 0.090f;
        float sLineStep = 0.042f;
        float sValX = bPanelL + 0.32f;

        auto drawSummaryParam = [&](const std::string& name, const std::string& val, XMFLOAT4 valCol = { 0.95f, 0.96f, 1.00f, 0.95f }) {
            AddStringLines(name, bPanelL + 0.03f, sLineY, paramW, paramH, paramSp, dimText, uiVerts);
            AddStringLines(val, sValX, sLineY, paramW, paramH, paramSp, valCol, uiVerts);
            sLineY -= sLineStep;
        };

        const auto& curBst = internal.GetCurrentBooster();
        const auto& curGen = internal.GetCurrentGenerator();
        float totalWeight = frames.GetTotalWeight() + internal.GetTotalInternalWeight();
        float weightRatio = totalWeight / 8500.0f;
        float mobilityScale = std::clamp(1.20f - 0.20f * weightRatio, 0.75f, 1.25f);
        float totalEn = curGen.energyCapacity + frames.GetSlot(FrameSlot::Core).data.energyCapacity;

        drawSummaryParam("TOTAL ARMOR (AP)", std::to_string(static_cast<int>(frames.GetTotalAp())), green);
        drawSummaryParam("TOTAL ACS STABILITY", std::to_string(static_cast<int>(frames.GetTotalMaxAcs())), amber);
        drawSummaryParam("TOTAL WEIGHT", std::to_string(static_cast<int>(totalWeight)) + " KG", white);
        drawSummaryParam("MAX ENERGY (EN)", std::to_string(static_cast<int>(totalEn)), cyan);
        drawSummaryParam("CRUISE BOOST SPEED", std::to_string(static_cast<int>(curBst.thrustSpeed * mobilityScale)) + " M/S", white);
        drawSummaryParam("QUICK BOOST THRUST", std::to_string(static_cast<int>(curBst.qbThrust * mobilityScale)) + " M/S", amber);
        drawSummaryParam("QB EN DRAIN", std::to_string(static_cast<int>(curBst.qbEnergyCost)) + " EN", orange);
        drawSummaryParam("EN RECHARGE RATE", std::to_string(static_cast<int>(curGen.rechargeRate)) + " EN/S", green);

        // (D) Bottom Navigation Bar
        float bBoxL = -0.92f, bBoxR = 0.92f;
        float bBoxT = -0.74f, bBoxB = -0.88f;
        addRect(bBoxL, bBoxT, bBoxR, bBoxB, dimCyan);

        std::string navGuide = "[Q]/[E]: TAB   [W]/[S]: SLOT   [A]/[D]: CYCLE   [ENTER]: SORTIE   [ESC]: MENU";
        float gCharH = 0.019f;
        float gCharW = gCharH * invAspect * 0.70f;
        float gSp = gCharW * 0.32f;
        float gTotalW = navGuide.length() * (gCharW + gSp);
        AddStringLines(navGuide, -0.88f, -0.825f, gCharW, gCharH, gSp, cyan, uiVerts);

        std::string saveStatus = "[AUTO-SAVED: loadout.ini]";
        float asCharH = 0.017f;
        float asCharW = asCharH * invAspect * 0.70f;
        float asSp = asCharW * 0.30f;
        float asTotalW = saveStatus.length() * (asCharW + asSp);
        AddStringLines(saveStatus, 0.88f - asTotalW, -0.825f, asCharW, asCharH, asSp, green, uiVerts);

        // Upload and draw UI
        if (!uiVerts.empty())
        {
            D3D11_MAPPED_SUBRESOURCE dynMap;
            if (SUCCEEDED(m_context->Map(m_dynamicReticleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dynMap)))
            {
                UINT copyCount = static_cast<UINT>(std::min(uiVerts.size(), size_t(4096)));
                memcpy(dynMap.pData, uiVerts.data(), sizeof(Vertex) * copyCount);
                m_context->Unmap(m_dynamicReticleBuffer.Get(), 0);

                UINT stride = sizeof(Vertex);
                UINT offset = 0;
                m_context->IASetVertexBuffers(0, 1, m_dynamicReticleBuffer.GetAddressOf(), &stride, &offset);
                m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                m_context->Draw(copyCount, 0);
            }
        }

        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 1);
    }

    void D3D11Renderer::RenderVRMainMenu(
        const MenuSystem& menu,
        const XMMATRIX& view,
        const XMMATRIX& proj
    )
    {
        // 3D VR Holographic Menu Plane positioned 2.0 meters forward in cockpit space
        m_context->OMSetDepthStencilState(m_hudDepthDisabledState.Get(), 0);

        XMMATRIX world = XMMatrixTranslation(0.0f, 0.0f, 2.0f);

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = world;
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        std::vector<Vertex> uiVerts;
        uiVerts.reserve(4096);

        float animTime = menu.GetAnimTime();
        XMFLOAT4 cyan    = { 0.25f, 0.90f, 1.00f, 0.95f };
        XMFLOAT4 dimCyan = { 0.15f, 0.45f, 0.60f, 0.70f };
        XMFLOAT4 amber   = { 1.00f, 0.78f, 0.18f, 0.95f };
        XMFLOAT4 white   = { 0.95f, 0.96f, 1.00f, 0.95f };
        XMFLOAT4 dimText = { 0.60f, 0.68f, 0.75f, 0.85f };

        float pulse = 0.75f + 0.25f * std::sin(animTime * 6.5f);
        XMFLOAT4 activeCol = { 1.00f, 0.82f, 0.18f, pulse };

        auto addLine = [&](float x1, float y1, float x2, float y2, XMFLOAT4 col) {
            uiVerts.push_back({ { x1, y1, 0.0f }, { 0, 0, 1 }, col });
            uiVerts.push_back({ { x2, y2, 0.0f }, { 0, 0, 1 }, col });
        };

        auto addRect = [&](float l, float t, float r, float b, XMFLOAT4 col) {
            addLine(l, t, r, t, col);
            addLine(r, t, r, b, col);
            addLine(r, b, l, b, col);
            addLine(l, b, l, t, col);
        };

        // Title
        std::string mainTitle = "OVERDRIVE CORE";
        float tCharH = 0.060f, tCharW = 0.038f, tSp = 0.015f;
        float tTotalW = mainTitle.length() * (tCharW + tSp);
        AddStringLines(mainTitle, -tTotalW * 0.5f, 0.42f, tCharW, tCharH, tSp, white, uiVerts);

        std::string subTitle = "VR COMBAT SYSTEM";
        float sCharH = 0.025f, sCharW = 0.016f, sSp = 0.007f;
        float sTotalW = subTitle.length() * (sCharW + sSp);
        AddStringLines(subTitle, -sTotalW * 0.5f, 0.35f, sCharW, sCharH, sSp, cyan, uiVerts);

        // Menu items
        const auto& items = menu.GetItems();
        int selectedIdx = menu.GetSelectedIndex();
        float startY = 0.18f;
        float itemHeight = 0.10f;
        float mCharH = 0.035f, mCharW = 0.022f, mSp = 0.009f;
        float boxW = 0.80f;
        float boxL = -boxW * 0.5f, boxR = boxW * 0.5f;

        for (size_t i = 0; i < items.size(); ++i)
        {
            float itemY = startY - i * itemHeight;
            bool isSel = (static_cast<int>(i) == selectedIdx);

            if (isSel)
            {
                addRect(boxL, itemY + 0.035f, boxR, itemY - 0.035f, activeCol);
                AddStringLines("> ", boxL + 0.03f, itemY, mCharW, mCharH, mSp, activeCol, uiVerts);
            }
            else
            {
                addLine(boxL, itemY - 0.035f, boxR, itemY - 0.035f, dimCyan);
            }

            XMFLOAT4 itemCol = isSel ? white : (items[i].isEnabled ? dimText : XMFLOAT4(0.4f, 0.4f, 0.4f, 0.5f));
            AddStringLines(items[i].label, boxL + 0.09f, itemY, mCharW, mCharH, mSp, itemCol, uiVerts);
        }

        // Description
        const MenuItem* curItem = menu.GetSelectedItem();
        if (curItem)
        {
            float descY = -0.30f;
            addLine(boxL, descY + 0.04f, boxR, descY + 0.04f, dimCyan);
            addLine(boxL, descY - 0.04f, boxR, descY - 0.04f, dimCyan);
            std::string d = "INFO: " + curItem->description;
            float dCharH = 0.022f, dCharW = 0.014f, dSp = 0.006f;
            float dTotalW = d.length() * (dCharW + dSp);
            AddStringLines(d, -dTotalW * 0.5f, descY, dCharW, dCharH, dSp, amber, uiVerts);
        }

        // Draw VR UI
        if (!uiVerts.empty())
        {
            D3D11_MAPPED_SUBRESOURCE dynMap;
            if (SUCCEEDED(m_context->Map(m_dynamicReticleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dynMap)))
            {
                UINT copyCount = static_cast<UINT>(std::min(uiVerts.size(), size_t(4096)));
                memcpy(dynMap.pData, uiVerts.data(), sizeof(Vertex) * copyCount);
                m_context->Unmap(m_dynamicReticleBuffer.Get(), 0);

                UINT stride = sizeof(Vertex);
                UINT offset = 0;
                m_context->IASetVertexBuffers(0, 1, m_dynamicReticleBuffer.GetAddressOf(), &stride, &offset);
                m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                m_context->Draw(copyCount, 0);
            }
        }

        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 1);
    }

    void D3D11Renderer::RenderVRDirectConnectMenu(
        const std::string& inputIp,
        const std::string& myTailscaleIp,
        uint16_t port,
        float animTime,
        const std::string& notificationMessage,
        const XMMATRIX& view,
        const XMMATRIX& proj
    )
    {
        m_context->OMSetDepthStencilState(m_hudDepthDisabledState.Get(), 0);

        XMMATRIX world = XMMatrixTranslation(0.0f, 0.0f, 2.0f);

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            auto* cb = static_cast<TransformConstantBuffer*>(mapped.pData);
            cb->world = world;
            cb->view = view;
            cb->projection = proj;
            cb->customParams = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
            m_context->Unmap(m_constantBuffer.Get(), 0);
        }

        std::vector<Vertex> uiVerts;
        uiVerts.reserve(4096);

        XMFLOAT4 cyan    = { 0.25f, 0.90f, 1.00f, 0.95f };
        XMFLOAT4 dimCyan = { 0.15f, 0.45f, 0.60f, 0.70f };
        XMFLOAT4 amber   = { 1.00f, 0.78f, 0.18f, 0.95f };
        XMFLOAT4 white   = { 0.95f, 0.96f, 1.00f, 0.95f };
        XMFLOAT4 dimText = { 0.60f, 0.68f, 0.75f, 0.85f };
        XMFLOAT4 green   = { 0.20f, 0.95f, 0.45f, 0.95f };

        float pulse = 0.75f + 0.25f * std::sin(animTime * 6.5f);
        XMFLOAT4 activeCol = { 1.00f, 0.82f, 0.18f, pulse };

        auto addLine = [&](float x1, float y1, float x2, float y2, XMFLOAT4 col) {
            uiVerts.push_back({ { x1, y1, 0.0f }, { 0, 0, 1 }, col });
            uiVerts.push_back({ { x2, y2, 0.0f }, { 0, 0, 1 }, col });
        };

        auto addRect = [&](float l, float t, float r, float b, XMFLOAT4 col) {
            addLine(l, t, r, t, col);
            addLine(r, t, r, b, col);
            addLine(r, b, l, b, col);
            addLine(l, b, l, t, col);
        };

        // Title
        std::string title = "DIRECT IP CONNECT";
        float tCharH = 0.055f, tCharW = 0.035f, tSp = 0.012f;
        float tTotalW = title.length() * (tCharW + tSp);
        AddStringLines(title, -tTotalW * 0.5f, 0.44f, tCharW, tCharH, tSp, white, uiVerts);

        std::string sub = "TAILSCALE / VPN / LAN";
        float sCharH = 0.022f, sCharW = 0.014f, sSp = 0.006f;
        float sTotalW = sub.length() * (sCharW + sSp);
        AddStringLines(sub, -sTotalW * 0.5f, 0.38f, sCharW, sCharH, sSp, cyan, uiVerts);

        // Center IP Box
        float boxW = 0.80f;
        float boxL = -boxW * 0.5f, boxR = boxW * 0.5f;
        addRect(boxL, 0.32f, boxR, 0.08f, dimCyan);

        std::string prompt = "ENTER HOST IP (PORT: " + std::to_string(port) + "):";
        AddStringLines(prompt, boxL + 0.04f, 0.27f, sCharW, sCharH, sSp, cyan, uiVerts);

        // Input field
        addRect(boxL + 0.03f, 0.22f, boxR - 0.03f, 0.11f, activeCol);
        std::string displayIp = inputIp;
        if (fmod(animTime, 0.8f) < 0.4f) displayIp += "_";
        else displayIp += " ";
        AddStringLines(displayIp, boxL + 0.06f, 0.165f, 0.028f, 0.044f, 0.010f, white, uiVerts);

        // Guide
        std::string pasteGuide = "[CTRL+V] PASTE IP    [0-9/.] TYPE    [BS] DELETE";
        AddStringLines(pasteGuide, boxL + 0.04f, 0.04f, 0.011f, 0.017f, 0.005f, amber, uiVerts);

        // Self IP Box
        addRect(boxL, -0.02f, boxR, -0.22f, dimCyan);
        std::string myIpTitle = "YOUR DETECTED IP (TAILSCALE / LAN):";
        AddStringLines(myIpTitle, boxL + 0.04f, -0.07f, sCharW, sCharH, sSp, cyan, uiVerts);
        std::string myIpStr = myTailscaleIp + ":" + std::to_string(port);
        AddStringLines(myIpStr, boxL + 0.04f, -0.13f, 0.022f, 0.032f, 0.008f, green, uiVerts);
        std::string hint = "PRESS [C] TO COPY YOUR IP TO SHARE";
        AddStringLines(hint, boxL + 0.04f, -0.18f, 0.010f, 0.015f, 0.004f, dimText, uiVerts);

        // Notification
        if (!notificationMessage.empty())
        {
            float nTotalW = notificationMessage.length() * (0.013f + 0.005f);
            AddStringLines(notificationMessage, -nTotalW * 0.5f, -0.28f, 0.013f, 0.020f, 0.005f, activeCol, uiVerts);
        }

        // Footer
        std::string footer = "[ENTER/A] CONNECT    [ESC/B] CANCEL";
        float fTotalW = footer.length() * (0.014f + 0.006f);
        AddStringLines(footer, -fTotalW * 0.5f, -0.38f, 0.014f, 0.022f, 0.006f, cyan, uiVerts);

        // Draw VR UI
        if (!uiVerts.empty())
        {
            D3D11_MAPPED_SUBRESOURCE dynMap;
            if (SUCCEEDED(m_context->Map(m_dynamicReticleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &dynMap)))
            {
                UINT copyCount = static_cast<UINT>(std::min(uiVerts.size(), size_t(4096)));
                memcpy(dynMap.pData, uiVerts.data(), sizeof(Vertex) * copyCount);
                m_context->Unmap(m_dynamicReticleBuffer.Get(), 0);

                UINT stride = sizeof(Vertex);
                UINT offset = 0;
                m_context->IASetVertexBuffers(0, 1, m_dynamicReticleBuffer.GetAddressOf(), &stride, &offset);
                m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
                m_context->Draw(copyCount, 0);
            }
        }

        m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 1);
    }

    void D3D11Renderer::EndFrame(bool vsync)
    {
        m_swapChain->Present(vsync ? 1 : 0, 0);
    }
}
