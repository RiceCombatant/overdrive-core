#include "ModelLoader.hpp"
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace Overdrive
{
    bool GLTFModel::LoadFromFile(ID3D11Device* device, const std::string& filepath)
    {
        m_filePath = filepath;
        m_submeshes.clear();

        if (!std::filesystem::exists(filepath))
        {
            return false;
        }

        fastgltf::Parser parser;
        auto data = fastgltf::GltfDataBuffer::FromPath(std::filesystem::path(filepath));
        if (data.error() != fastgltf::Error::None)
        {
            std::cerr << "[MODEL LOADER] Failed to load data from: " << filepath << std::endl;
            return false;
        }

        constexpr auto options = fastgltf::Options::DontRequireValidAssetMember 
                               | fastgltf::Options::LoadGLBBuffers 
                               | fastgltf::Options::LoadExternalBuffers;

        auto assetResult = parser.loadGltf(data.get(), std::filesystem::path(filepath).parent_path(), options);
        if (assetResult.error() != fastgltf::Error::None)
        {
            std::cerr << "[MODEL LOADER] Parse error in " << filepath << " (code: " 
                      << fastgltf::to_underlying(assetResult.error()) << ")" << std::endl;
            return false;
        }

        auto& asset = assetResult.get();

        bool firstVertex = true;
        XMFLOAT3 minPt = { 0, 0, 0 };
        XMFLOAT3 maxPt = { 0, 0, 0 };

        for (const auto& mesh : asset.meshes)
        {
            for (const auto& primitive : mesh.primitives)
            {
                // Must have POSITION attribute
                auto posIt = primitive.findAttribute("POSITION");
                if (posIt == primitive.attributes.end()) continue;

                auto& posAccessor = asset.accessors[posIt->accessorIndex];
                size_t vertCount = posAccessor.count;
                if (vertCount == 0) continue;

                std::vector<Vertex> vertices(vertCount);

                // 1. Read Positions & Calculate Bounds
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, posAccessor,
                    [&](fastgltf::math::fvec3 pos, size_t idx) {
                        float x = pos.x();
                        float y = pos.y();
                        float z = pos.z();
                        vertices[idx].position = XMFLOAT3(x, y, z);
                        vertices[idx].normal   = XMFLOAT3(0.0f, 1.0f, 0.0f);
                        vertices[idx].color    = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

                        if (firstVertex)
                        {
                            minPt = maxPt = XMFLOAT3(x, y, z);
                            firstVertex = false;
                        }
                        else
                        {
                            minPt.x = std::min(minPt.x, x);
                            minPt.y = std::min(minPt.y, y);
                            minPt.z = std::min(minPt.z, z);
                            maxPt.x = std::max(maxPt.x, x);
                            maxPt.y = std::max(maxPt.y, y);
                            maxPt.z = std::max(maxPt.z, z);
                        }
                    });

                // 2. Read Normals (if available)
                auto normIt = primitive.findAttribute("NORMAL");
                bool hasNormals = false;
                if (normIt != primitive.attributes.end())
                {
                    hasNormals = true;
                    auto& normAccessor = asset.accessors[normIt->accessorIndex];
                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, normAccessor,
                        [&](fastgltf::math::fvec3 norm, size_t idx) {
                            if (idx < vertCount)
                            {
                                vertices[idx].normal = XMFLOAT3(norm.x(), norm.y(), norm.z());
                            }
                        });
                }

                // 3. Read Colors (if available)
                auto colIt = primitive.findAttribute("COLOR_0");
                if (colIt != primitive.attributes.end())
                {
                    auto& colAccessor = asset.accessors[colIt->accessorIndex];
                    if (colAccessor.type == fastgltf::AccessorType::Vec4)
                    {
                        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, colAccessor,
                            [&](fastgltf::math::fvec4 c, size_t idx) {
                                if (idx < vertCount)
                                    vertices[idx].color = XMFLOAT4(c.x(), c.y(), c.z(), c.w());
                            });
                    }
                    else if (colAccessor.type == fastgltf::AccessorType::Vec3)
                    {
                        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, colAccessor,
                            [&](fastgltf::math::fvec3 c, size_t idx) {
                                if (idx < vertCount)
                                    vertices[idx].color = XMFLOAT4(c.x(), c.y(), c.z(), 1.0f);
                            });
                    }
                }

                // 4. Material BaseColorFactor
                XMFLOAT4 baseColor = { 0.88f, 0.90f, 0.94f, 1.0f };
                if (primitive.materialIndex.has_value())
                {
                    size_t matIdx = primitive.materialIndex.value();
                    if (matIdx < asset.materials.size())
                    {
                        const auto& mat = asset.materials[matIdx];
                        const auto& bc = mat.pbrData.baseColorFactor;
                        baseColor = XMFLOAT4(bc[0], bc[1], bc[2], bc[3]);
                    }
                }

                // Apply baseColor to vertices if no vertex colors existed
                if (colIt == primitive.attributes.end())
                {
                    for (auto& v : vertices)
                    {
                        v.color = baseColor;
                    }
                }

                // 5. Read Indices
                std::vector<uint32_t> indices;
                if (primitive.indicesAccessor.has_value())
                {
                    auto& indAccessor = asset.accessors[primitive.indicesAccessor.value()];
                    indices.reserve(indAccessor.count);
                    fastgltf::iterateAccessor<uint32_t>(asset, indAccessor,
                        [&](uint32_t ind) {
                            indices.push_back(ind);
                        });
                }

                // Compute flat normals if model didn't provide normals
                if (!hasNormals && !indices.empty())
                {
                    for (size_t i = 0; i + 2 < indices.size(); i += 3)
                    {
                        uint32_t i0 = indices[i];
                        uint32_t i1 = indices[i + 1];
                        uint32_t i2 = indices[i + 2];
                        if (i0 < vertCount && i1 < vertCount && i2 < vertCount)
                        {
                            XMVECTOR p0 = XMLoadFloat3(&vertices[i0].position);
                            XMVECTOR p1 = XMLoadFloat3(&vertices[i1].position);
                            XMVECTOR p2 = XMLoadFloat3(&vertices[i2].position);
                            XMVECTOR n = XMVector3Normalize(XMVector3Cross(p1 - p0, p2 - p0));
                            XMFLOAT3 fn;
                            XMStoreFloat3(&fn, n);
                            vertices[i0].normal = fn;
                            vertices[i1].normal = fn;
                            vertices[i2].normal = fn;
                        }
                    }
                }

                // Create Direct3D 11 Buffers
                GLTFSubmesh submesh;
                submesh.indexCount = static_cast<UINT>(indices.size());
                submesh.vertexCount = static_cast<UINT>(vertices.size());
                submesh.baseColor = baseColor;
                submesh.isIndexed = !indices.empty();

                D3D11_BUFFER_DESC vbd = {};
                vbd.Usage = D3D11_USAGE_DEFAULT;
                vbd.ByteWidth = sizeof(Vertex) * static_cast<UINT>(vertices.size());
                vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                D3D11_SUBRESOURCE_DATA vInit = {};
                vInit.pSysMem = vertices.data();
                HRESULT hr = device->CreateBuffer(&vbd, &vInit, submesh.vertexBuffer.GetAddressOf());
                if (FAILED(hr))
                {
                    std::cerr << "[MODEL LOADER] Failed to create Vertex Buffer for submesh" << std::endl;
                    continue;
                }

                if (submesh.isIndexed)
                {
                    D3D11_BUFFER_DESC ibd = {};
                    ibd.Usage = D3D11_USAGE_DEFAULT;
                    ibd.ByteWidth = sizeof(uint32_t) * static_cast<UINT>(indices.size());
                    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
                    D3D11_SUBRESOURCE_DATA iInit = {};
                    iInit.pSysMem = indices.data();
                    hr = device->CreateBuffer(&ibd, &iInit, submesh.indexBuffer.GetAddressOf());
                    if (FAILED(hr))
                    {
                        std::cerr << "[MODEL LOADER] Failed to create Index Buffer for submesh" << std::endl;
                        continue;
                    }
                }

                m_submeshes.push_back(std::move(submesh));
            }
        }

        m_minBounds = minPt;
        m_maxBounds = maxPt;
        m_center = XMFLOAT3(
            (minPt.x + maxPt.x) * 0.5f,
            (minPt.y + maxPt.y) * 0.5f,
            (minPt.z + maxPt.z) * 0.5f
        );
        m_size = XMFLOAT3(
            std::max(0.001f, maxPt.x - minPt.x),
            std::max(0.001f, maxPt.y - minPt.y),
            std::max(0.001f, maxPt.z - minPt.z)
        );

        std::cout << "[MODEL LOADER] Successfully loaded GLB: " << filepath 
                  << " (" << m_submeshes.size() << " submeshes, Size: " 
                  << m_size.x << "x" << m_size.y << "x" << m_size.z << ")" << std::endl;

        return !m_submeshes.empty();
    }

    void GLTFModel::Render(
        ID3D11DeviceContext* context,
        ID3D11Buffer* constantBuffer,
        const XMMATRIX& world,
        const XMFLOAT4& tintColor
    )
    {
        if (m_submeshes.empty()) return;

        UINT stride = sizeof(Vertex);
        UINT offset = 0;

        for (const auto& submesh : m_submeshes)
        {
            context->IASetVertexBuffers(0, 1, submesh.vertexBuffer.GetAddressOf(), &stride, &offset);

            if (submesh.isIndexed && submesh.indexBuffer)
            {
                context->IASetIndexBuffer(submesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
                context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                context->DrawIndexed(submesh.indexCount, 0, 0);
            }
            else
            {
                context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                context->Draw(submesh.vertexCount, 0);
            }
        }
    }

    std::shared_ptr<GLTFModel> ModelManager::GetOrLoad(ID3D11Device* device, const std::string& filepath)
    {
        auto it = m_cache.find(filepath);
        if (it != m_cache.end())
        {
            return it->second;
        }

        if (!std::filesystem::exists(filepath))
        {
            return nullptr;
        }

        auto model = std::make_shared<GLTFModel>();
        if (model->LoadFromFile(device, filepath))
        {
            m_cache[filepath] = model;
            return model;
        }

        return nullptr;
    }

    bool ModelManager::HasModelFile(const std::string& filepath) const
    {
        return std::filesystem::exists(filepath);
    }

    void ModelManager::Clear()
    {
        m_cache.clear();
    }
}
