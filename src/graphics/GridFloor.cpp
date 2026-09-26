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

        // 2. Generate Tactical Combat Arena Structures
        // (Shelters with roofs, Mega blocks, Barricades, Giant pillars, and Slopes)
        std::vector<Vertex> wallVertices;
        std::vector<uint32_t> wallIndices;

        XMFLOAT4 wallColor       = { 0.18f, 0.20f, 0.24f, 1.0f }; // Perimeter boundary wall
        XMFLOAT4 pillarColor     = { 0.25f, 0.28f, 0.35f, 1.0f }; // Giant columns & support pillars
        XMFLOAT4 megaBlockColor  = { 0.22f, 0.26f, 0.32f, 1.0f }; // Large facility blocks & mega containers
        XMFLOAT4 coverColor      = { 0.28f, 0.34f, 0.42f, 1.0f }; // Tactical cover walls & barricades
        XMFLOAT4 roofColor       = { 0.32f, 0.36f, 0.44f, 1.0f }; // Protective overhead canopy roofs
        XMFLOAT4 rampColor       = { 0.22f, 0.40f, 0.54f, 1.0f }; // Slope ramp
        XMFLOAT4 hazardAmber     = { 0.88f, 0.65f, 0.15f, 1.0f }; // Industrial hazard warning trims

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

        // --- (A) Perimeter Boundary Walls (280m x 280m Arena) ---
        addBox(0.0f, 25.0f,  130.0f, 280.0f, 50.0f, 8.0f, wallColor);
        addBox(0.0f, 25.0f, -130.0f, 280.0f, 50.0f, 8.0f, wallColor);
        addBox( 130.0f, 25.0f, 0.0f, 8.0f, 50.0f, 280.0f, wallColor);
        addBox(-130.0f, 25.0f, 0.0f, 8.0f, 50.0f, 280.0f, wallColor);

        // Buttress pillars along North perimeter wall
        for (float px = -110.0f; px <= 110.0f; px += 22.0f)
        {
            addBox(px, 25.0f, 124.0f, 6.0f, 50.0f, 6.0f, pillarColor);
        }

        // --- (B) Tactical Shelters with Overhead Canopy Roofs (Protects against vertical missile / aerial bombardment) ---
        // Shelter 1: South-West Tactical Bunker (Walkable underneath, climbable roof)
        // Roof Canopy: 26m wide x 24m deep at Y=8.5m (Clearance ~7.9m)
        addBox(-18.0f, 8.5f, -15.0f, 26.0f, 1.2f, 24.0f, roofColor);
        // Roof hazard edge trims
        addBox(-18.0f, 9.15f, -26.9f, 26.0f, 0.2f, 0.4f, hazardAmber);
        addBox(-18.0f, 9.15f,  -3.1f, 26.0f, 0.2f, 0.4f, hazardAmber);
        // 4 Support Columns for Shelter 1
        addBox(-29.5f, 4.0f, -25.5f, 2.2f, 8.0f, 2.2f, pillarColor);
        addBox( -6.5f, 4.0f, -25.5f, 2.2f, 8.0f, 2.2f, pillarColor);
        addBox(-29.5f, 4.0f,  -4.5f, 2.2f, 8.0f, 2.2f, pillarColor);
        addBox( -6.5f, 4.0f,  -4.5f, 2.2f, 8.0f, 2.2f, pillarColor);

        // Shelter 2: North-East Hangar Gantry Shelter
        // Roof Canopy: 28m wide x 22m deep at Y=9.5m (Clearance ~8.9m)
        addBox(22.0f, 9.5f, 28.0f, 28.0f, 1.2f, 22.0f, roofColor);
        // Roof hazard edge trims
        addBox(22.0f, 10.15f, 17.1f, 28.0f, 0.2f, 0.4f, hazardAmber);
        addBox(22.0f, 10.15f, 38.9f, 28.0f, 0.2f, 0.4f, hazardAmber);
        // 4 Support Columns for Shelter 2
        addBox( 9.5f, 4.5f, 18.5f, 2.2f, 9.0f, 2.2f, pillarColor);
        addBox(34.5f, 4.5f, 18.5f, 2.2f, 9.0f, 2.2f, pillarColor);
        addBox( 9.5f, 4.5f, 37.5f, 2.2f, 9.0f, 2.2f, pillarColor);
        addBox(34.5f, 4.5f, 37.5f, 2.2f, 9.0f, 2.2f, pillarColor);

        // --- (C) Mega Blocks & Facility Containers (Large high-altitude climbable boxes) ---
        // 1. Central-East Mega Cargo Block (18m x 11m x 22m, top at Y=11.0m)
        addBox(38.0f, 5.5f, -12.0f, 18.0f, 11.0f, 22.0f, megaBlockColor);
        // Top edge trim
        addBox(38.0f, 11.05f, -1.1f, 17.6f, 0.2f, 0.4f, hazardAmber);

        // 2. North-West Heavy Facility Block (20m x 13m x 18m, top at Y=13.0m)
        addBox(-42.0f, 6.5f, 32.0f, 20.0f, 13.0f, 18.0f, megaBlockColor);

        // 3. South-West Stepped Container Stack (Allows staged jump to high tier)
        addBox(-28.0f, 2.5f, -48.0f, 16.0f, 5.0f, 12.0f, megaBlockColor); // Lower tier (5m)
        addBox(-31.0f, 7.5f, -48.0f, 10.0f, 5.0f, 8.0f, coverColor);      // Upper tier (10m)

        // 4. South-East Cargo Bunker (14m x 7m x 14m)
        addBox(45.0f, 3.5f, -45.0f, 14.0f, 7.0f, 14.0f, megaBlockColor);

        // --- (D) Tactical Cover Walls & Barricades (Ground-level breaking line-of-sight) ---
        // 1. Central L-Shaped Barricade (Crucial firefight choke point)
        addBox(-5.0f, 3.0f,  8.0f,  1.8f, 6.0f, 18.0f, coverColor); // North-South segment
        addBox( 9.0f, 3.0f, -0.5f, 14.0f, 6.0f,  1.8f, coverColor); // East-West segment (offset eastward to leave origin (0,0) corridor completely clear)

        // 2. North Trench Wall (22m wide x 6m high)
        addBox(-10.0f, 3.0f, 55.0f, 22.0f, 6.0f, 2.0f, coverColor);

        // 3. East Approach Barricade (18m wide x 5m high)
        addBox(16.0f, 2.5f, -36.0f, 18.0f, 5.0f, 2.0f, coverColor);

        // 4. South Perimeter Low Wall (16m wide x 4m high)
        addBox(-2.0f, 2.0f, -55.0f, 16.0f, 4.0f, 1.8f, coverColor);

        // 5. Mid-North Corner Bunker Wall
        addBox(2.0f, 2.5f, 24.0f, 10.0f, 5.0f, 1.8f, coverColor);

        // --- (E) Giant Columns & Power Pillars (For boost-slalom and circular dogfights) ---
        addBox(-12.0f, 14.0f, 26.0f, 4.5f, 28.0f, 4.5f, pillarColor); // Pillar Alpha (28m high)
        addBox( 14.0f, 14.0f,  2.0f, 4.5f, 28.0f, 4.5f, pillarColor); // Pillar Beta (28m high)
        addBox(-48.0f, 16.0f, -6.0f, 5.0f, 32.0f, 5.0f, pillarColor); // Pillar Gamma (32m high)
        addBox( 52.0f, 16.0f, 18.0f, 5.0f, 32.0f, 5.0f, pillarColor); // Pillar Delta (32m high)

        // --- (F) Integrated Slope & Elevated Sky Platform ---
        // 1. High-speed Boost Slope: 14m wide, 26m long, pitch ~ 14.5 deg (-0.25 rad)
        float rampPitch = 0.25f;
        addBox(-55.0f, 3.25f, 24.0f, 14.0f, 0.6f, 26.0f, rampColor, -rampPitch);

        // 2. High Elevated Sky Deck at the top of the slope (Y = 7.0m)
        addBox(-55.0f, 3.5f, 44.0f, 16.0f, 7.0f, 16.0f, megaBlockColor);

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

        // --- (A) Perimeter Boundary Walls ---
        physicsManager->CreateStaticBox({ 0.0f, 25.0f,  130.0f }, { 140.0f, 25.0f, 4.0f });
        physicsManager->CreateStaticBox({ 0.0f, 25.0f, -130.0f }, { 140.0f, 25.0f, 4.0f });
        physicsManager->CreateStaticBox({  130.0f, 25.0f, 0.0f }, { 4.0f, 25.0f, 140.0f });
        physicsManager->CreateStaticBox({ -130.0f, 25.0f, 0.0f }, { 4.0f, 25.0f, 140.0f });

        // North wall buttress pillars
        for (float px = -110.0f; px <= 110.0f; px += 22.0f)
        {
            physicsManager->CreateStaticBox({ px, 25.0f, 124.0f }, { 3.0f, 25.0f, 3.0f });
        }

        // --- (B) Tactical Shelters with Overhead Canopy Roofs ---
        // Shelter 1: South-West Bunker Roof (Solid physical roof blocks vertical shells / missiles)
        physicsManager->CreateStaticBox({ -18.0f, 8.5f, -15.0f }, { 13.0f, 0.6f, 12.0f });
        // Shelter 1: 4 Support Columns
        physicsManager->CreateStaticBox({ -29.5f, 4.0f, -25.5f }, { 1.1f, 4.0f, 1.1f });
        physicsManager->CreateStaticBox({  -6.5f, 4.0f, -25.5f }, { 1.1f, 4.0f, 1.1f });
        physicsManager->CreateStaticBox({ -29.5f, 4.0f,  -4.5f }, { 1.1f, 4.0f, 1.1f });
        physicsManager->CreateStaticBox({  -6.5f, 4.0f,  -4.5f }, { 1.1f, 4.0f, 1.1f });

        // Shelter 2: North-East Hangar Gantry Roof
        physicsManager->CreateStaticBox({ 22.0f, 9.5f, 28.0f }, { 14.0f, 0.6f, 11.0f });
        // Shelter 2: 4 Support Columns
        physicsManager->CreateStaticBox({  9.5f, 4.5f, 18.5f }, { 1.1f, 4.5f, 1.1f });
        physicsManager->CreateStaticBox({ 34.5f, 4.5f, 18.5f }, { 1.1f, 4.5f, 1.1f });
        physicsManager->CreateStaticBox({  9.5f, 4.5f, 37.5f }, { 1.1f, 4.5f, 1.1f });
        physicsManager->CreateStaticBox({ 34.5f, 4.5f, 37.5f }, { 1.1f, 4.5f, 1.1f });

        // --- (C) Mega Blocks & Facility Containers ---
        // 1. Central-East Mega Cargo Block (18m x 11m x 22m)
        physicsManager->CreateStaticBox({ 38.0f, 5.5f, -12.0f }, { 9.0f, 5.5f, 11.0f });

        // 2. North-West Heavy Facility Block (20m x 13m x 18m)
        physicsManager->CreateStaticBox({ -42.0f, 6.5f, 32.0f }, { 10.0f, 6.5f, 9.0f });

        // 3. South-West Stepped Container Stack
        physicsManager->CreateStaticBox({ -28.0f, 2.5f, -48.0f }, { 8.0f, 2.5f, 6.0f }); // Lower
        physicsManager->CreateStaticBox({ -31.0f, 7.5f, -48.0f }, { 5.0f, 2.5f, 4.0f }); // Upper

        // 4. South-East Cargo Bunker
        physicsManager->CreateStaticBox({ 45.0f, 3.5f, -45.0f }, { 7.0f, 3.5f, 7.0f });

        // --- (D) Tactical Cover Walls & Barricades ---
        // 1. Central L-Shaped Barricade
        physicsManager->CreateStaticBox({ -5.0f, 3.0f,  8.0f }, { 0.9f, 3.0f, 9.0f });
        physicsManager->CreateStaticBox({  9.0f, 3.0f, -0.5f }, { 7.0f, 3.0f, 0.9f });

        // 2. North Trench Wall
        physicsManager->CreateStaticBox({ -10.0f, 3.0f, 55.0f }, { 11.0f, 3.0f, 1.0f });

        // 3. East Approach Barricade
        physicsManager->CreateStaticBox({ 16.0f, 2.5f, -36.0f }, { 9.0f, 2.5f, 1.0f });

        // 4. South Perimeter Low Wall
        physicsManager->CreateStaticBox({ -2.0f, 2.0f, -55.0f }, { 8.0f, 2.0f, 0.9f });

        // 5. Mid-North Corner Bunker Wall
        physicsManager->CreateStaticBox({ 2.0f, 2.5f, 24.0f }, { 5.0f, 2.5f, 0.9f });

        // --- (E) Giant Columns & Power Pillars ---
        physicsManager->CreateStaticBox({ -12.0f, 14.0f, 26.0f }, { 2.25f, 14.0f, 2.25f });
        physicsManager->CreateStaticBox({  14.0f, 14.0f,  2.0f }, { 2.25f, 14.0f, 2.25f });
        physicsManager->CreateStaticBox({ -48.0f, 16.0f, -6.0f }, { 2.5f,  16.0f, 2.5f });
        physicsManager->CreateStaticBox({  52.0f, 16.0f, 18.0f }, { 2.5f,  16.0f, 2.5f });

        // --- (F) Integrated Slope & Elevated Sky Platform ---
        float rampPitch = -0.25f;
        float qx = std::sin(rampPitch * 0.5f);
        float qw = std::cos(rampPitch * 0.5f);
        physicsManager->CreateStaticBox({ -55.0f, 3.25f, 24.0f }, { 7.0f, 0.3f, 13.0f }, { qx, 0.0f, 0.0f, qw });
        physicsManager->CreateStaticBox({ -55.0f, 3.5f,  44.0f }, { 8.0f, 3.5f, 8.0f });

        std::cout << "[PHYSICS] Tactical combat arena colliders (Shelters, Roofs, Mega Blocks, Cover Walls, Pillars) initialized in Jolt Physics." << std::endl;
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

    void GridFloor::RenderFloorOnly(ID3D11DeviceContext* context)
    {
        UINT stride = sizeof(Vertex);
        UINT offset = 0;

        // Glowing cyber grid floor only (clean unobstructed hangar for Assemble and Main Menu)
        context->IASetVertexBuffers(0, 1, m_lineVertexBuffer.GetAddressOf(), &stride, &offset);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        context->Draw(m_lineVertexCount, 0);
    }
}
