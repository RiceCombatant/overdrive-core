#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <memory>
#include <DirectXMath.h>

namespace Overdrive
{
    namespace Layers
    {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;
        static constexpr JPH::ObjectLayer MOVING     = 1;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    }

    namespace BroadPhaseLayers
    {
        static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
        static constexpr JPH::BroadPhaseLayer MOVING(1);
        static constexpr uint32_t NUM_LAYERS = 2;
    }

    class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
    {
    public:
        BPLayerInterfaceImpl();
        virtual uint32_t GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif

    private:
        JPH::BroadPhaseLayer m_objectToBroadPhase[Layers::NUM_LAYERS];
    };

    class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
    };

    class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
    };

    class PhysicsManager
    {
    public:
        PhysicsManager();
        ~PhysicsManager();

        bool Initialize();
        void Update(float deltaTime);

        // Body creation helpers for terrain/obstacles
        JPH::BodyID CreateStaticBox(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& halfExtent, const DirectX::XMFLOAT4& rotationQuat = { 0, 0, 0, 1 });

        JPH::PhysicsSystem* GetPhysicsSystem() { return m_physicsSystem.get(); }
        JPH::TempAllocator* GetTempAllocator() { return m_tempAllocator.get(); }

    private:
        std::unique_ptr<JPH::TempAllocatorImpl> m_tempAllocator;
        std::unique_ptr<JPH::JobSystemThreadPool> m_jobSystem;
        std::unique_ptr<JPH::PhysicsSystem> m_physicsSystem;

        BPLayerInterfaceImpl m_bpLayerInterface;
        ObjectVsBroadPhaseLayerFilterImpl m_objectVsBroadPhaseFilter;
        ObjectLayerPairFilterImpl m_objectLayerPairFilter;
    };
}
