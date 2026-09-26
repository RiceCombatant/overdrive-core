#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "math/MathTypes.hpp"

namespace Overdrive
{
    using Microsoft::WRL::ComPtr;
    using namespace DirectX;

    struct GLTFSubmesh
    {
        ComPtr<ID3D11Buffer> vertexBuffer;
        ComPtr<ID3D11Buffer> indexBuffer;
        UINT indexCount = 0;
        UINT vertexCount = 0;
        XMFLOAT4 baseColor = { 0.85f, 0.88f, 0.92f, 1.0f };
        bool isIndexed = true;
    };

    class GLTFModel
    {
    public:
        GLTFModel() = default;
        ~GLTFModel() = default;

        // Load GLB (glTF Binary) or glTF file from disk and upload to D3D11 buffers
        bool LoadFromFile(ID3D11Device* device, const std::string& filepath);

        // Render all submeshes with transform and tint color
        void Render(
            ID3D11DeviceContext* context,
            ID3D11Buffer* constantBuffer,
            const XMMATRIX& world,
            const XMFLOAT4& tintColor = { 1.0f, 1.0f, 1.0f, 1.0f }
        );

        bool IsValid() const { return !m_submeshes.empty(); }
        const std::string& GetFilePath() const { return m_filePath; }

        // AABB & Dimensional Info
        const XMFLOAT3& GetMinBounds() const { return m_minBounds; }
        const XMFLOAT3& GetMaxBounds() const { return m_maxBounds; }
        const XMFLOAT3& GetCenter() const { return m_center; }
        const XMFLOAT3& GetSize() const { return m_size; }

    private:
        std::string m_filePath;
        std::vector<GLTFSubmesh> m_submeshes;
        XMFLOAT3 m_minBounds = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_maxBounds = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_center = { 0.0f, 0.0f, 0.0f };
        XMFLOAT3 m_size = { 1.0f, 1.0f, 1.0f };
    };

    class ModelManager
    {
    public:
        ModelManager() = default;
        ~ModelManager() = default;

        // Retrieve cached model or load from disk if not yet cached
        // Returns nullptr if file doesn't exist or loading fails (allows clean fallback to procedural geometry)
        std::shared_ptr<GLTFModel> GetOrLoad(ID3D11Device* device, const std::string& filepath);

        // Check if model file exists on disk
        bool HasModelFile(const std::string& filepath) const;

        // Unload/clear cache
        void Clear();

    private:
        std::unordered_map<std::string, std::shared_ptr<GLTFModel>> m_cache;
    };
}
