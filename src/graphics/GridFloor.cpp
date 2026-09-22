#include "GridFloor.hpp"
#include "physics/PhysicsManager.hpp"
#include <iostream>
#include <cmath>

namespace Overdrive
{
    GridFloor::GridFloor()
    {
    }

    bool GridFloor::Initialize(ID3D11Device* device)
    {
        // 1. Generate Cyber Grid lines (AC6 style glowing floor)
        std::vector<Vertex> lineVertices;

        const float extent = 200.0f;
        const float spacing = 4.0f;
        const int count = static_cast<int>(extent / spacing);

        XMFLOAT4 subGridColor  = { 0.12f, 0.35f, 0.60f, 1.0f }; // Soft cyber blue
        XMFLOAT4 mainGridColor = { 0.35f, 0.80f, 1.00f, 1.0f }; // Bright glowing cyan

        for (int i = -count; i <= count; ++i)
        {
            float coord = i * spacing;
            bool isMajor = (i % 5 == 0);
            XMFLOAT4 color = isMajor ? mainGridColor : subGridColor;

            // X lines
            lineVertices.push_back({ { coord, 0.01f, -extent }, { 0.0f, 1.0f, 0.0f }, color });
            lineVertices.push_back({ { coord, 0.01f,  extent }, { 0.0f, 1.0f, 0.0f }, color });

            // Z lines
            lineVertices.push_back({ { -extent, 0.01f, coord }, { 0.0f, 1.0f, 0.0f }, color });
            lineVertices.push_back({ {  extent, 0.01f, coord }, { 0.0f, 1.0f, 0.0f }, color });
        }

        m_lineVertexCount = static_cast<UINT>(lineVertices.size());

        D3D11_BUFFER_DESC vbd = {};
        vbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * lineVertices.size());
        vbd.Usage = D3D11_USAGE_IMMUTABLE;
        vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vdata = {};
        vdata.pSysMem = lineVertices.data();

        HRESULT hr = device->CreateBuffer(&vbd, &vdata, m_lineVertexBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        // 2. Generate Hangar Walls, Pillars & Test Obstacles (Ramps / Blocks)
        std::vector<Vertex> wallVertices;
        std::vector<uint32_t> wallIndices;

        XMFLOAT4 wallColor     = { 0.20f, 0.22f, 0.25f, 1.0f };
        XMFLOAT4 pillarColor   = { 0.28f, 0.30f, 0.35f, 1.0f };
        XMFLOAT4 obstacleColor = { 0.30f, 0.34f, 0.40f, 1.0f };
        XMFLOAT4 rampColor     = { 0.25f, 0.42f, 0.55f, 1.0f }; // Blue-tinted ramp

        auto addBox = [&](float cx, float cy, float cz, float sx, float sy, float sz, XMFLOAT4 col, float pitchRad = 0.0f) {
            uint32_t baseIdx = static_cast<uint32_t>(wallVertices.size());
            float hx = sx * 0.5f, hy = sy * 0.5f, hz = sz * 0.5f;

            XMFLOAT3 localCorners[8] = {
                { -hx, -hy, -hz }, { -hx,  hy, -hz }, {  hx,  hy, -hz }, {  hx, -hy, -hz },
                {  hx, -hy,  hz }, {  hx,  hy,  hz }, { -hx,  hy,  hz }, { -hx, -hy,  hz }
            };

            float cosP = std::cos(pitchRad);
            float sinP = std::sin(pitchRad);

            for (int i = 0; i < 8; ++i)
            {
                // Pitch rotation around X axis
                float y = localCorners[i].y * cosP - localCorners[i].z * sinP;
                float z = localCorners[i].y * sinP + localCorners[i].z * cosP;
                float x = localCorners[i].x;

                wallVertices.push_back({ { cx + x, cy + y, cz + z }, { 0.0f, 1.0f, 0.0f }, col });
            }

            uint32_t boxIndices[] = {
                0, 1, 2,  0, 2, 3,
                4, 5, 6,  4, 6, 7,
                1, 6, 5,  1, 5, 2,
                7, 0, 3,  7, 3, 4,
                7, 6, 1,  7, 1, 0,
                3, 2, 5,  3, 5, 4
            };

            for (uint32_t idx : boxIndices)
            {
                wallIndices.push_back(baseIdx + idx);
            }
        };

        // Boundary walls
        addBox(0.0f, 25.0f, 130.0f, 280.0f, 50.0f, 8.0f, wallColor);
        addBox(0.0f, 25.0f, -130.0f, 280.0f, 50.0f, 8.0f, wallColor);
        addBox(130.0f, 25.0f, 0.0f, 8.0f, 50.0f, 280.0f, wallColor);
        addBox(-130.0f, 25.0f, 0.0f, 8.0f, 50.0f, 280.0f, wallColor);

        // Pillars along the back wall
        for (float px = -110.0f; px <= 110.0f; px += 22.0f)
        {
            addBox(px, 25.0f, 124.0f, 6.0f, 50.0f, 6.0f, pillarColor);
        }

        // --- Test Obstacles & Slopes ---
        // 1. Test Ramp / Slope: 16m wide, 28m long, smoothly submerged into the floor at Y=0
        // Pitch ~ 14.5 degrees (0.25 rad), seamless entry from floor to 6.5m high platform
        float rampPitch = 0.25f;
        addBox(-25.0f, 3.0f, 25.5f, 16.0f, 0.6f, 28.0f, rampColor, -rampPitch);

        // 2. High Elevated Platform at the top of the ramp
        addBox(-25.0f, 3.25f, 48.0f, 18.0f, 6.5f, 18.0f, obstacleColor);

        // 3. Low Cover Blocks for QB Dash & Collision Impact Test
        addBox(20.0f, 1.5f, 25.0f, 10.0f, 3.0f, 4.0f, obstacleColor);
        addBox(35.0f, 2.5f, 40.0f, 8.0f, 5.0f, 8.0f, obstacleColor);

        m_wallIndexCount = static_cast<UINT>(wallIndices.size());

        D3D11_BUFFER_DESC wbd = {};
        wbd.ByteWidth = static_cast<UINT>(sizeof(Vertex) * wallVertices.size());
        wbd.Usage = D3D11_USAGE_IMMUTABLE;
        wbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA wvdata = {};
        wvdata.pSysMem = wallVertices.data();

        hr = device->CreateBuffer(&wbd, &wvdata, m_wallVertexBuffer.GetAddressOf());
        if (FAILED(hr)) return false;

        D3D11_BUFFER_DESC wibd = {};
        wibd.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * wallIndices.size());
        wibd.Usage = D3D11_USAGE_IMMUTABLE;
        wibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA widata = {};
        widata.pSysMem = wallIndices.data();

        hr = device->CreateBuffer(&wibd, &widata, m_wallIndexBuffer.GetAddressOf());
        return SUCCEEDED(hr);
    }

    bool GridFloor::InitializePhysics(PhysicsManager* physicsManager)
    {
        if (!physicsManager) return false;

        // 1. Static Ground Plane (Floor at Y = 0)
        physicsManager->CreateStaticBox({ 0.0f, -1.0f, 0.0f }, { 200.0f, 1.0f, 200.0f });

        // 2. Arena Boundary Walls
        physicsManager->CreateStaticBox({ 0.0f, 25.0f,  130.0f }, { 140.0f, 25.0f, 4.0f });
        physicsManager->CreateStaticBox({ 0.0f, 25.0f, -130.0f }, { 140.0f, 25.0f, 4.0f });
        physicsManager->CreateStaticBox({  130.0f, 25.0f, 0.0f }, { 4.0f, 25.0f, 140.0f });
        physicsManager->CreateStaticBox({ -130.0f, 25.0f, 0.0f }, { 4.0f, 25.0f, 140.0f });

        // 3. Pillars
        for (float px = -110.0f; px <= 110.0f; px += 22.0f)
        {
            physicsManager->CreateStaticBox({ px, 25.0f, 124.0f }, { 3.0f, 25.0f, 3.0f });
        }

        // 4. Test Ramp (Slope): Seamlessly matches the mesh
        float rampPitch = -0.25f;
        float qx = std::sin(rampPitch * 0.5f);
        float qw = std::cos(rampPitch * 0.5f);
        physicsManager->CreateStaticBox({ -25.0f, 3.0f, 25.5f }, { 8.0f, 0.3f, 14.0f }, { qx, 0.0f, 0.0f, qw });

        // 5. Elevated Platform & Cover Blocks
        physicsManager->CreateStaticBox({ -25.0f, 3.25f, 48.0f }, { 9.0f, 3.25f, 9.0f });
        physicsManager->CreateStaticBox({ 20.0f, 1.5f, 25.0f }, { 5.0f, 1.5f, 2.0f });
        physicsManager->CreateStaticBox({ 35.0f, 2.5f, 40.0f }, { 4.0f, 2.5f, 4.0f });

        std::cout << "[PHYSICS] Static scene colliders (Ground, Walls, Pillars, Slopes) created in Jolt Physics." << std::endl;
        return true;
    }

    void GridFloor::Render(ID3D11DeviceContext* context)
    {
        UINT stride = sizeof(Vertex);
        UINT offset = 0;

        // Glowing cyber grid lines
        context->IASetVertexBuffers(0, 1, m_lineVertexBuffer.GetAddressOf(), &stride, &offset);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        context->Draw(m_lineVertexCount, 0);

        // Hangar walls, pillars, slopes, and obstacle platforms
        context->IASetVertexBuffers(0, 1, m_wallVertexBuffer.GetAddressOf(), &stride, &offset);
        context->IASetIndexBuffer(m_wallIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->DrawIndexed(m_wallIndexCount, 0, 0);
    }
}
