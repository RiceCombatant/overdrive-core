#pragma once

#include <DirectXMath.h>

namespace Overdrive
{
    using namespace DirectX;

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT4 color;
    };

    struct UIVertex
    {
        XMFLOAT2 position;
        XMFLOAT4 color;
    };

    struct alignas(16) TransformConstantBuffer
    {
        XMMATRIX world;
        XMMATRIX view;
        XMMATRIX projection;
        XMFLOAT4 customParams; // x: useLighting (1 = lit, 0 = unlit/emissive), yzw: reserved
    };
}
