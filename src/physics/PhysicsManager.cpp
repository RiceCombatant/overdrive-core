#include "PhysicsManager.hpp"
#include <iostream>
#include <thread>

namespace Overdrive
{
    BPLayerInterfaceImpl::BPLayerInterfaceImpl()
    {
        m_objectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        m_objectToBroadPhase[Layers::MOVING]     = BroadPhaseLayers::MOVING;
    }

    JPH::BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return m_objectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:     return "MOVING";
        default:                                                      return "INVALID";
        }
    }
#endif

    bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
    {
        switch (inLayer1)
        {
        case Layers::NON_MOVING:
            return inLayer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            return false;
        }
    }

    bool ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
    {
        switch (inObject1)
        {
        case Layers::NON_MOVING:
            return inObject2 == Layers::MOVING;
        case Layers::MOVING:
            return true;
        default:
            return false;
        }
    }

    PhysicsManager::PhysicsManager()
    {
    }

    PhysicsManager::~PhysicsManager()
    {
        if (m_physicsSystem)
        {
            m_physicsSystem.reset();
        }
        m_jobSystem.reset();
        m_tempAllocator.reset();

        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    bool PhysicsManager::Initialize()
    {
        // 1. Initialize Allocators & Factory
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        // 10MB Temp Allocator for contact points and narrow phase
        m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);

        // JobSystem using hardware thread pool
        int numThreads = std::max(1u, std::thread::hardware_concurrency() - 1);
        m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, numThreads);

        // 2. Initialize Physics System
        const uint32_t cMaxBodies = 1024;
        const uint32_t cNumBodyMutexes = 0;
        const uint32_t cMaxBodyPairs = 1024;
        const uint32_t cMaxContactConstraints = 1024;

        m_physicsSystem = std::make_unique<JPH::PhysicsSystem>();
        m_physicsSystem->Init(
            cMaxBodies,
            cNumBodyMutexes,
            cMaxBodyPairs,
            cMaxContactConstraints,
            m_bpLayerInterface,
            m_objectVsBroadPhaseFilter,
            m_objectLayerPairFilter
        );

        // AC6 standard gravity (-28 m/s^2 for snappy vertical drops)
        m_physicsSystem->SetGravity(JPH::Vec3(0.0f, -28.0f, 0.0f));

        std::cout << "[PHYSICS] Jolt Physics System initialized with " << numThreads << " worker threads." << std::endl;
        return true;
    }

    void PhysicsManager::Update(float deltaTime)
    {
        if (!m_physicsSystem) return;

        // Step simulation (typically 60Hz: 1/60s per step)
        const int cCollisionSteps = 1;
        m_physicsSystem->Update(deltaTime, cCollisionSteps, m_tempAllocator.get(), m_jobSystem.get());
    }

    JPH::BodyID PhysicsManager::CreateStaticBox(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& halfExtent, const DirectX::XMFLOAT4& rotationQuat)
    {
        JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();

        JPH::BoxShapeSettings shapeSettings(JPH::Vec3(halfExtent.x, halfExtent.y, halfExtent.z));
        shapeSettings.SetEmbedded();

        JPH::ShapeSettings::ShapeResult result = shapeSettings.Create();
        if (result.HasError())
        {
            std::cerr << "[PHYSICS] Error creating box shape: " << result.GetError() << std::endl;
            return JPH::BodyID();
        }

        JPH::BodyCreationSettings bodySettings(
            result.Get(),
            JPH::RVec3(position.x, position.y, position.z),
            JPH::Quat(rotationQuat.x, rotationQuat.y, rotationQuat.z, rotationQuat.w),
            JPH::EMotionType::Static,
            Layers::NON_MOVING
        );
        bodySettings.mFriction = 0.5f;
        bodySettings.mRestitution = 0.0f;

        JPH::BodyID bodyId = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::DontActivate);
        return bodyId;
    }
}
