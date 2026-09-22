#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include "math/MathTypes.hpp"

using Microsoft::WRL::ComPtr;

namespace Overdrive
{
    class PhysicsManager;

    class GridFloor
    {
    public:
        GridFloor();
        bool Initialize(ID3D11Device* device);
        bool InitializePhysics(PhysicsManager* physicsManager);
        void Render(ID3D11DeviceContext* context);

    private:
        ComPtr<ID3D11Buffer> m_lineVertexBuffer;
        UINT m_lineVertexCount = 0;

        ComPtr<ID3D11Buffer> m_wallVertexBuffer;
        ComPtr<ID3D11Buffer> m_wallIndexBuffer;
        UINT m_wallIndexCount = 0;
    };
}
