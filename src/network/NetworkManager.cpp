#include "network/NetworkManager.hpp"
#include "physics/PhysicsManager.hpp"
#include "audio/AudioManager.hpp"
#include "combat/WeaponSystem.hpp"
#include "core/MechController.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace Overdrive
{
    // =========================================================================
    // RemoteMech Implementation
    // =========================================================================
    RemoteMech::RemoteMech()
    {
        m_currentPos = { 0.0f, -100.0f, 0.0f };
        m_targetPos  = { 0.0f, -100.0f, 0.0f };
    }

    RemoteMech::~RemoteMech()
    {
    }

    void RemoteMech::SetPlayerId(uint8_t id)
    {
        m_playerId = id;
        switch (id)
        {
        case 0:
            m_playerName = "PLAYER 1 (HOST)";
            m_primaryColor   = { 0.88f, 0.18f, 0.20f, 1.0f }; // Crimson Red
            m_secondaryColor = { 0.22f, 0.24f, 0.28f, 1.0f }; // Dark Titanium
            break;
        case 1:
            m_playerName = "PLAYER 2";
            m_primaryColor   = { 0.15f, 0.50f, 0.95f, 1.0f }; // Cobalt Blue
            m_secondaryColor = { 0.85f, 0.88f, 0.92f, 1.0f }; // Silver White
            break;
        case 2:
            m_playerName = "PLAYER 3";
            m_primaryColor   = { 0.95f, 0.65f, 0.10f, 1.0f }; // Amber Gold
            m_secondaryColor = { 0.18f, 0.18f, 0.22f, 1.0f }; // Dark Charcoal
            break;
        case 3:
        default:
            m_playerName = "PLAYER 4";
            m_primaryColor   = { 0.15f, 0.85f, 0.45f, 1.0f }; // Emerald Green
            m_secondaryColor = { 0.35f, 0.15f, 0.45f, 1.0f }; // Dark Violet
            break;
        }
    }

    void RemoteMech::InitializePhysics(PhysicsManager* physics)
    {
        if (!physics) return;

        // Create Static Box collider for remote opponent mech (width: 1.8m, height: 2.5m, depth: 1.8m)
        // Center offset Y=1.25m so origin is at feet
        m_bodyId = physics->CreateStaticBox({ 0.0f, -100.0f, 0.0f }, { 0.9f, 1.25f, 0.9f });
        std::cout << "[NETWORK] Remote mech " << m_playerName << " physics collider registered in Jolt." << std::endl;
    }

    void RemoteMech::Update(float deltaTime, PhysicsManager* physics)
    {
        if (!m_isActive) return;

        if (m_hitFlashTimer > 0.0f)
        {
            m_hitFlashTimer -= deltaTime;
        }

        if (m_isStaggered)
        {
            m_staggerTimer -= deltaTime;
            if (m_staggerTimer <= 0.0f)
            {
                m_isStaggered = false;
                m_currentAcs = 0.0f;
                m_staggerTimer = 0.0f;
            }
        }
        else if (m_currentAcs > 0.0f)
        {
            if (m_acsCooldown > 0.0f)
            {
                m_acsCooldown -= deltaTime;
            }
            else
            {
                m_currentAcs = std::max(0.0f, m_currentAcs - deltaTime * 320.0f);
            }
        }

        if (m_isDestroyed)
        {
            m_respawnTimer -= deltaTime;
            if (m_respawnTimer <= 0.0f)
            {
                m_isDestroyed = false;
                m_isStaggered = false;
                m_currentAcs = 0.0f;
                m_currentHp = m_maxHp;
            }
        }

        // Smoothly interpolate current transform towards target state (Dead-reckoning & smoothing)
        float lerpSpeed = 22.0f;
        float t = std::clamp(lerpSpeed * deltaTime, 0.0f, 1.0f);

        m_currentPos.x += (m_targetPos.x - m_currentPos.x) * t;
        m_currentPos.y += (m_targetPos.y - m_currentPos.y) * t;
        m_currentPos.z += (m_targetPos.z - m_currentPos.z) * t;

        // Angle wrapping interpolation for yaw
        float diffYaw = m_targetYaw - m_currentYaw;
        while (diffYaw > XM_PI)  diffYaw -= XM_2PI;
        while (diffYaw < -XM_PI) diffYaw += XM_2PI;
        m_currentYaw += diffYaw * t;

        m_currentPitch += (m_targetPitch - m_currentPitch) * t;
        m_currentRoll  += (m_targetRoll  - m_currentRoll) * t;

        // Update Jolt physics collider position
        if (physics && !m_bodyId.IsInvalid())
        {
            JPH::BodyInterface& bodyInterface = physics->GetPhysicsSystem()->GetBodyInterface();
            if (m_isDestroyed || !m_isActive)
            {
                bodyInterface.SetPosition(m_bodyId, JPH::RVec3(0.0f, -100.0f, 0.0f), JPH::EActivation::DontActivate);
            }
            else
            {
                // Collider center is Y + 1.25m above mech feet
                JPH::RVec3 newPos(m_currentPos.x, m_currentPos.y + 1.25f, m_currentPos.z);
                JPH::Quat newRot = JPH::Quat::sRotation(JPH::Vec3::sAxisY(), m_currentYaw);
                bodyInterface.SetPositionAndRotation(m_bodyId, newPos, newRot, JPH::EActivation::DontActivate);
            }
        }
    }

    void RemoteMech::ApplyState(const MechStatePacket& packet)
    {
        m_targetPos = { packet.posX, packet.posY, packet.posZ };
        m_velocity  = { packet.velX, packet.velY, packet.velZ };

        m_targetYaw   = packet.yaw;
        m_targetPitch = packet.pitch;
        m_targetRoll  = packet.roll;

        m_currentHp   = packet.hp;

        m_boostOn     = (packet.flags & 1) != 0;
        m_isQB        = (packet.flags & 2) != 0;
        m_isAB        = (packet.flags & 4) != 0;
        m_isGrounded  = (packet.flags & 8) != 0;
        if ((packet.flags & 16) != 0)
        {
            m_isStaggered = true;
            m_currentAcs = m_maxAcs;
        }
        m_isBoostKick = (packet.flags & 32) != 0;

        // Remote mech revived/respawned by sender: clear destroyed state immediately
        if (m_currentHp > 0.0f && m_isDestroyed)
        {
            m_isDestroyed = false;
            m_isStaggered = false;
            m_currentAcs = 0.0f;
            m_respawnTimer = 0.0f;
            m_currentPos = m_targetPos;
            m_currentYaw = m_targetYaw;
            m_currentPitch = m_targetPitch;
            m_currentRoll = m_targetRoll;
        }
        else if (m_currentHp <= 0.0f && !m_isDestroyed)
        {
            m_isDestroyed = true;
            m_isStaggered = false;
            m_currentAcs = 0.0f;
            m_respawnTimer = 3.0f;
        }

        // Distance check for large teleport / snap (e.g. initial connection or respawn)
        float dx = m_targetPos.x - m_currentPos.x;
        float dy = m_targetPos.y - m_currentPos.y;
        float dz = m_targetPos.z - m_currentPos.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (!m_isActive || !m_hasReceivedFirstPacket || distSq > (14.0f * 14.0f))
        {
            m_isActive = true;
            m_hasReceivedFirstPacket = true;
            m_currentPos = m_targetPos;
            m_currentYaw = m_targetYaw;
            m_currentPitch = m_targetPitch;
            m_currentRoll = m_targetRoll;
        }
    }

    void RemoteMech::TakeDamage(float damage, float impact, float directHitMult, AudioManager* audio)
    {
        float finalDmg = damage;
        if (m_isStaggered)
        {
            finalDmg = damage * directHitMult;
        }
        else
        {
            m_currentAcs += impact;
            m_acsCooldown = 2.2f;
            if (m_currentAcs >= m_maxAcs)
            {
                m_isStaggered = true;
                m_currentAcs = m_maxAcs;
                m_staggerTimer = c_staggerDuration;
                if (audio)
                {
                    audio->PlayStaggerBreak(m_currentPos);
                }
            }
        }

        m_currentHp = std::max(0.0f, m_currentHp - finalDmg);
        m_hitFlashTimer = 0.15f;
        if (m_currentHp <= 0.0f && !m_isDestroyed)
        {
            m_isDestroyed = true;
            m_isStaggered = false;
            m_currentAcs = 0.0f;
            m_respawnTimer = 3.0f;
        }
    }

    void RemoteMech::Respawn(const XMFLOAT3& spawnPos, float yaw)
    {
        m_currentHp = m_maxHp;
        m_isDestroyed = false;
        m_isStaggered = false;
        m_currentAcs = 0.0f;
        m_staggerTimer = 0.0f;
        m_currentPos = spawnPos;
        m_targetPos = spawnPos;
        m_currentYaw = yaw;
        m_targetYaw = yaw;
        m_velocity = { 0.0f, 0.0f, 0.0f };
        m_hasReceivedFirstPacket = true;
    }

    void RemoteMech::ApplyKnockback(const XMFLOAT3& direction, float force)
    {
        float len = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
        if (len > 0.001f)
        {
            m_velocity.x += (direction.x / len) * force;
            m_velocity.y += (direction.y / len) * (force * 0.35f) + 4.5f;
            m_velocity.z += (direction.z / len) * force;
            m_targetPos.x += (direction.x / len) * (force * 0.25f);
            m_targetPos.z += (direction.z / len) * (force * 0.25f);
            m_isGrounded = false;
        }
    }

    // =========================================================================
    // NetworkManager Implementation (4-Player Star Topology)
    // =========================================================================
    NetworkManager::NetworkManager()
    {
        m_remoteMechs.resize(MAX_PLAYERS);
        for (uint8_t i = 0; i < MAX_PLAYERS; ++i)
        {
            m_remoteMechs[i].SetPlayerId(i);
        }
    }

    NetworkManager::~NetworkManager()
    {
        Shutdown();
    }

    bool NetworkManager::Initialize()
    {
        if (m_initialized) return true;

        WSADATA wsaData;
        int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (err != 0)
        {
            std::cerr << "[NETWORK ERROR] WSAStartup failed: " << err << std::endl;
            return false;
        }

        m_initialized = true;
        std::cout << "[NETWORK] WinSock2 Subsystem Initialized for 4-Player Battle." << std::endl;
        return true;
    }

    void NetworkManager::Shutdown()
    {
        if (m_socket != INVALID_SOCKET)
        {
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
        }

        if (m_initialized)
        {
            WSACleanup();
            m_initialized = false;
        }

        m_state = NetworkState::Offline;
        m_role = NetworkRole::None;
        for (auto& slot : m_clientSlots)
        {
            slot.active = false;
        }
        for (auto& mech : m_remoteMechs)
        {
            mech.SetActive(false);
        }
    }

    void NetworkManager::InitializeRemotePhysics(PhysicsManager* physics)
    {
        for (auto& mech : m_remoteMechs)
        {
            mech.InitializePhysics(physics);
        }
    }

    bool NetworkManager::StartHost(uint16_t port)
    {
        if (!Initialize()) return false;

        if (m_socket != INVALID_SOCKET)
        {
            closesocket(m_socket);
        }

        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_socket == INVALID_SOCKET)
        {
            std::cerr << "[NETWORK ERROR] Host socket creation failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        // Set non-blocking mode
        u_long nonBlocking = 1;
        ioctlsocket(m_socket, FIONBIO, &nonBlocking);

        sockaddr_in bindAddr = {};
        bindAddr.sin_family = AF_INET;
        bindAddr.sin_port = htons(port);
        bindAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(m_socket, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) == SOCKET_ERROR)
        {
            std::cerr << "[NETWORK ERROR] Host bind to port " << port << " failed: " << WSAGetLastError() << std::endl;
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            return false;
        }

        m_role = NetworkRole::Host;
        m_state = NetworkState::Hosting;
        m_port = port;
        m_localPlayerId = 0; // Host is always Player 0

        for (auto& slot : m_clientSlots)
        {
            slot.active = false;
            slot.timeoutTimer = 0.0f;
        }

        std::cout << "========================================================" << std::endl;
        std::cout << " [NETWORK] 4-PLAYER HOST MODE STARTED! Port: " << port << std::endl;
        std::cout << " [NETWORK] You are PLAYER 1 (Crimson Red)" << std::endl;
        std::cout << " [NETWORK] Waiting for players 2, 3, and 4 to connect..." << std::endl;
        std::cout << "========================================================" << std::endl;
        return true;
    }

    bool NetworkManager::StartClient(const std::string& hostIp, uint16_t port)
    {
        if (!Initialize()) return false;

        if (m_socket != INVALID_SOCKET)
        {
            closesocket(m_socket);
        }

        m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_socket == INVALID_SOCKET)
        {
            std::cerr << "[NETWORK ERROR] Client socket creation failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        // Set non-blocking mode
        u_long nonBlocking = 1;
        ioctlsocket(m_socket, FIONBIO, &nonBlocking);

        m_hostAddr = {};
        m_hostAddr.sin_family = AF_INET;
        m_hostAddr.sin_port = htons(port);
        inet_pton(AF_INET, hostIp.c_str(), &m_hostAddr.sin_addr);

        m_hasHostAddr = true;
        m_role = NetworkRole::Client;
        m_state = NetworkState::Connecting;
        m_targetIp = hostIp;
        m_port = port;
        m_hostTimeoutTimer = 0.0f;

        // Send Handshake Request to Host
        HandshakePacket req = {};
        req.header.magic = NET_MAGIC;
        req.header.type = PacketType::HandshakeReq;
        req.header.senderId = 0xFF; // Unassigned

        SendPacketTo(&req, sizeof(req), m_hostAddr);

        std::cout << "========================================================" << std::endl;
        std::cout << " [NETWORK] CONNECTING TO HOST: " << hostIp << ":" << port << std::endl;
        std::cout << "========================================================" << std::endl;
        return true;
    }

    int NetworkManager::GetConnectedPlayerCount() const
    {
        int count = 1; // Self
        if (m_role == NetworkRole::Host)
        {
            for (const auto& slot : m_clientSlots)
            {
                if (slot.active) count++;
            }
        }
        else if (m_role == NetworkRole::Client && m_state == NetworkState::Connected)
        {
            for (const auto& mech : m_remoteMechs)
            {
                if (mech.IsActive()) count++;
            }
        }
        return count;
    }

    std::string NetworkManager::GetStatusString() const
    {
        switch (m_state)
        {
        case NetworkState::Offline:
            return "OFFLINE (Press H:Host / C:Join)";
        case NetworkState::Hosting:
            return "HOSTING: " + std::to_string(GetConnectedPlayerCount()) + "/4 PLAYERS (Port " + std::to_string(m_port) + ")";
        case NetworkState::Connecting:
            return "CONNECTING TO " + m_targetIp + ":" + std::to_string(m_port) + "...";
        case NetworkState::Connected:
            return "ONLINE: PLAYER " + std::to_string(m_localPlayerId + 1) + " (" + std::to_string(GetConnectedPlayerCount()) + "/4 ACTIVE)";
        case NetworkState::Disconnected:
            return "DISCONNECTED (HOST TIMEOUT)";
        default:
            return "UNKNOWN";
        }
    }

    RemoteMech& NetworkManager::GetRemoteMech()
    {
        // Backward compatibility: return first active remote mech, or slot 1
        for (auto& mech : m_remoteMechs)
        {
            if (mech.GetPlayerId() != m_localPlayerId && mech.IsActive())
            {
                return mech;
            }
        }
        return m_remoteMechs[m_localPlayerId == 0 ? 1 : 0];
    }

    const RemoteMech& NetworkManager::GetRemoteMech() const
    {
        for (const auto& mech : m_remoteMechs)
        {
            if (mech.GetPlayerId() != m_localPlayerId && mech.IsActive())
            {
                return mech;
            }
        }
        return m_remoteMechs[m_localPlayerId == 0 ? 1 : 0];
    }

    void NetworkManager::SendPacketTo(const void* data, int size, const sockaddr_in& targetAddr)
    {
        if (m_socket == INVALID_SOCKET) return;
        sendto(m_socket, reinterpret_cast<const char*>(data), size, 0,
               reinterpret_cast<const sockaddr*>(&targetAddr), sizeof(targetAddr));
    }

    void NetworkManager::RelayPacket(const void* data, int size, const sockaddr_in& senderAddr)
    {
        // Host relays to all active clients EXCEPT the sender
        for (const auto& slot : m_clientSlots)
        {
            if (slot.active)
            {
                if (slot.addr.sin_addr.s_addr != senderAddr.sin_addr.s_addr ||
                    slot.addr.sin_port != senderAddr.sin_port)
                {
                    SendPacketTo(data, size, slot.addr);
                }
            }
        }
    }

    void NetworkManager::SendStatePacket(
        const XMFLOAT3& localPos,
        const XMFLOAT3& localVel,
        float localYaw,
        float localPitch,
        float localRoll,
        float localHp,
        bool isBoost,
        bool isQB,
        bool isAB,
        bool isGrounded,
        bool isStaggered,
        bool isBoostKick)
    {
        MechStatePacket pkt = {};
        pkt.header.magic = NET_MAGIC;
        pkt.header.type = PacketType::MechState;
        pkt.header.senderId = m_localPlayerId;
        pkt.sequence = ++m_sendSequence;

        pkt.posX = localPos.x;
        pkt.posY = localPos.y;
        pkt.posZ = localPos.z;

        pkt.velX = localVel.x;
        pkt.velY = localVel.y;
        pkt.velZ = localVel.z;

        pkt.yaw   = localYaw;
        pkt.pitch = localPitch;
        pkt.roll  = localRoll;
        pkt.hp    = localHp;

        pkt.flags = 0;
        if (isBoost)     pkt.flags |= 1;
        if (isQB)        pkt.flags |= 2;
        if (isAB)        pkt.flags |= 4;
        if (isGrounded)  pkt.flags |= 8;
        if (isStaggered) pkt.flags |= 16;
        if (isBoostKick) pkt.flags |= 32;

        if (m_role == NetworkRole::Host)
        {
            // Send to all connected clients
            for (const auto& slot : m_clientSlots)
            {
                if (slot.active)
                {
                    SendPacketTo(&pkt, sizeof(pkt), slot.addr);
                }
            }
        }
        else if (m_role == NetworkRole::Client && m_hasHostAddr)
        {
            // Send to host
            SendPacketTo(&pkt, sizeof(pkt), m_hostAddr);
        }
    }

    void NetworkManager::SendFireEvent(bool isLeftArm, const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos)
    {
        FireEventPacket pkt = {};
        pkt.header.magic = NET_MAGIC;
        pkt.header.type = PacketType::FireEvent;
        pkt.header.senderId = m_localPlayerId;
        pkt.isLeftArm = isLeftArm ? 1 : 0;
        pkt.muzzleX = muzzlePos.x;
        pkt.muzzleY = muzzlePos.y;
        pkt.muzzleZ = muzzlePos.z;
        pkt.targetX = targetPos.x;
        pkt.targetY = targetPos.y;
        pkt.targetZ = targetPos.z;

        if (m_role == NetworkRole::Host)
        {
            for (const auto& slot : m_clientSlots)
            {
                if (slot.active)
                {
                    SendPacketTo(&pkt, sizeof(pkt), slot.addr);
                }
            }
        }
        else if (m_role == NetworkRole::Client && m_hasHostAddr)
        {
            SendPacketTo(&pkt, sizeof(pkt), m_hostAddr);
        }
    }

    void NetworkManager::SendHitEvent(uint8_t targetPlayerId, float damage, const XMFLOAT3& hitPos, float impact, float directHitMult)
    {
        HitEventPacket pkt = {};
        pkt.header.magic = NET_MAGIC;
        pkt.header.type = PacketType::HitEvent;
        pkt.header.senderId = m_localPlayerId;
        pkt.targetPlayerId = targetPlayerId;
        pkt.damage = damage;
        pkt.hitX = hitPos.x;
        pkt.hitY = hitPos.y;
        pkt.hitZ = hitPos.z;
        pkt.impact = impact;
        pkt.directHitMult = directHitMult;

        if (m_role == NetworkRole::Host)
        {
            for (const auto& slot : m_clientSlots)
            {
                if (slot.active)
                {
                    SendPacketTo(&pkt, sizeof(pkt), slot.addr);
                }
            }
        }
        else if (m_role == NetworkRole::Client && m_hasHostAddr)
        {
            SendPacketTo(&pkt, sizeof(pkt), m_hostAddr);
        }
    }

    void NetworkManager::BroadcastArenaMode(uint8_t mode)
    {
        m_syncedArenaMode = mode;
        ArenaModePacket pkt = {};
        pkt.header.magic = NET_MAGIC;
        pkt.header.type = PacketType::ArenaMode;
        pkt.header.senderId = m_localPlayerId;
        pkt.arenaMode = mode;

        if (m_role == NetworkRole::Host)
        {
            for (const auto& slot : m_clientSlots)
            {
                if (slot.active)
                {
                    SendPacketTo(&pkt, sizeof(pkt), slot.addr);
                }
            }
        }
        else if (m_role == NetworkRole::Client && m_hasHostAddr)
        {
            SendPacketTo(&pkt, sizeof(pkt), m_hostAddr);
        }
    }

    void NetworkManager::Update(
        float deltaTime,
        MechController& localMech,
        WeaponSystem* weapons,
        AudioManager* audio,
        PhysicsManager* physics)
    {
        Update(
            deltaTime,
            localMech.GetPosition(),
            localMech.GetVelocity(),
            localMech.GetYaw(),
            localMech.GetPitch(),
            localMech.GetRoll(),
            localMech.GetHp(),
            localMech.IsBoostMode(),
            localMech.IsQuickBoost(),
            localMech.IsAssaultBoost(),
            localMech.IsGrounded(),
            weapons,
            audio,
            physics,
            &localMech,
            localMech.IsStaggered(),
            localMech.IsBoostKicking()
        );
    }

    void NetworkManager::Update(
        float deltaTime,
        const XMFLOAT3& localPos,
        const XMFLOAT3& localVel,
        float localYaw,
        float localPitch,
        float localRoll,
        float localHp,
        bool isBoost,
        bool isQB,
        bool isAB,
        bool isGrounded,
        WeaponSystem* weapons,
        AudioManager* audio,
        PhysicsManager* physics,
        MechController* localMech,
        bool isStaggered,
        bool isBoostKick)
    {
        if (m_state == NetworkState::Offline) return;

        // 1. Process all incoming UDP packets
        ProcessIncomingPackets(weapons, audio, physics, localMech);

        // 2. Update all active remote mechs
        for (auto& mech : m_remoteMechs)
        {
            if (mech.GetPlayerId() != m_localPlayerId && mech.IsActive())
            {
                mech.Update(deltaTime, physics);
            }
        }

        // 3. Client timeout handling
        if (m_role == NetworkRole::Host)
        {
            for (auto& slot : m_clientSlots)
            {
                if (slot.active)
                {
                    slot.timeoutTimer += deltaTime;
                    if (slot.timeoutTimer > c_timeoutDuration)
                    {
                        std::cout << "[NETWORK] Player " << static_cast<int>(slot.playerId + 1) << " timed out / disconnected." << std::endl;
                        slot.active = false;
                        m_remoteMechs[slot.playerId].SetActive(false);
                    }
                }
            }
        }
        else if (m_role == NetworkRole::Client)
        {
            if (m_state == NetworkState::Connecting)
            {
                m_heartbeatTimer += deltaTime;
                if (m_heartbeatTimer > 0.5f)
                {
                    m_heartbeatTimer = 0.0f;
                    HandshakePacket req = {};
                    req.header.magic = NET_MAGIC;
                    req.header.type = PacketType::HandshakeReq;
                    req.header.senderId = 0xFF;
                    SendPacketTo(&req, sizeof(req), m_hostAddr);
                }
            }
            else if (m_state == NetworkState::Connected)
            {
                m_hostTimeoutTimer += deltaTime;
                if (m_hostTimeoutTimer > c_timeoutDuration)
                {
                    std::cout << "[NETWORK] Connection to Host lost (timeout)." << std::endl;
                    m_state = NetworkState::Disconnected;
                    for (auto& mech : m_remoteMechs)
                    {
                        mech.SetActive(false);
                    }
                }
            }
        }

        // 4. Send local player state (~60Hz)
        SendStatePacket(localPos, localVel, localYaw, localPitch, localRoll, localHp, isBoost, isQB, isAB, isGrounded, isStaggered, isBoostKick);
    }

    void NetworkManager::ProcessIncomingPackets(
        WeaponSystem* weapons,
        AudioManager* audio,
        PhysicsManager* physics,
        MechController* localMech)
    {
        if (m_socket == INVALID_SOCKET) return;

        char buffer[1024];
        sockaddr_in senderAddr = {};
        int senderAddrLen = sizeof(senderAddr);

        while (true)
        {
            int bytesRecv = recvfrom(m_socket, buffer, sizeof(buffer), 0,
                                     reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrLen);

            if (bytesRecv <= 0) break;
            if (bytesRecv < static_cast<int>(sizeof(PacketHeader))) continue;

            const auto* header = reinterpret_cast<const PacketHeader*>(buffer);
            if (header->magic != NET_MAGIC) continue;

            // =================================================================
            // Host logic: manage client connections and relay packets
            // =================================================================
            if (m_role == NetworkRole::Host)
            {
                // Identify which client slot this sender belongs to
                int clientIndex = -1;
                for (size_t i = 0; i < m_clientSlots.size(); ++i)
                {
                    if (m_clientSlots[i].active &&
                        m_clientSlots[i].addr.sin_addr.s_addr == senderAddr.sin_addr.s_addr &&
                        m_clientSlots[i].addr.sin_port == senderAddr.sin_port)
                    {
                        clientIndex = static_cast<int>(i);
                        break;
                    }
                }

                if (header->type == PacketType::HandshakeReq)
                {
                    // Find an available client slot (Player 1, 2, or 3)
                    if (clientIndex == -1)
                    {
                        for (size_t i = 0; i < m_clientSlots.size(); ++i)
                        {
                            if (!m_clientSlots[i].active)
                            {
                                clientIndex = static_cast<int>(i);
                                m_clientSlots[i].active = true;
                                m_clientSlots[i].addr = senderAddr;
                                m_clientSlots[i].playerId = static_cast<uint8_t>(i + 1); // 1, 2, 3
                                m_clientSlots[i].timeoutTimer = 0.0f;

                                XMFLOAT3 initPos = { 0.0f, 2.0f, 40.0f };
                                float initYaw = 3.14159265f;
                                if (m_clientSlots[i].playerId == 2) { initPos = { 40.0f, 2.0f, 0.0f }; initYaw = -1.5707963f; }
                                else if (m_clientSlots[i].playerId == 3) { initPos = { -40.0f, 2.0f, 0.0f }; initYaw = 1.5707963f; }

                                m_remoteMechs[m_clientSlots[i].playerId].Respawn(initPos, initYaw);
                                m_remoteMechs[m_clientSlots[i].playerId].SetActive(true);
                                m_state = NetworkState::Connected;

                                std::cout << "========================================================" << std::endl;
                                std::cout << " [NETWORK] NEW PLAYER JOINED! Slot " << (i + 1)
                                          << " assigned Player ID " << static_cast<int>(m_clientSlots[i].playerId + 1) << std::endl;
                                std::cout << "========================================================" << std::endl;
                                break;
                            }
                        }
                    }

                    if (clientIndex != -1)
                    {
                        HandshakePacket ack = {};
                        ack.header.magic = NET_MAGIC;
                        ack.header.type = PacketType::HandshakeAck;
                        ack.header.senderId = 0; // From Host
                        ack.assignedId = m_clientSlots[clientIndex].playerId;
                        SendPacketTo(&ack, sizeof(ack), senderAddr);

                        // Also send current ArenaMode to newly joined client
                        ArenaModePacket arenaPkt = {};
                        arenaPkt.header.magic = NET_MAGIC;
                        arenaPkt.header.type = PacketType::ArenaMode;
                        arenaPkt.header.senderId = 0;
                        arenaPkt.arenaMode = m_syncedArenaMode;
                        SendPacketTo(&arenaPkt, sizeof(arenaPkt), senderAddr);
                    }
                    continue;
                }

                if (clientIndex == -1) continue; // Unknown sender
                m_clientSlots[clientIndex].timeoutTimer = 0.0f;
                uint8_t senderPlayerId = m_clientSlots[clientIndex].playerId;

                // Relay packets to all other connected clients
                RelayPacket(buffer, bytesRecv, senderAddr);

                // Apply to local game simulation
                if (header->type == PacketType::MechState && bytesRecv >= static_cast<int>(sizeof(MechStatePacket)))
                {
                    const auto* statePkt = reinterpret_cast<const MechStatePacket*>(buffer);
                    m_remoteMechs[senderPlayerId].ApplyState(*statePkt);
                }
                else if (header->type == PacketType::ArenaMode && bytesRecv >= static_cast<int>(sizeof(ArenaModePacket)))
                {
                    const auto* arenaPkt = reinterpret_cast<const ArenaModePacket*>(buffer);
                    m_syncedArenaMode = arenaPkt->arenaMode;
                    m_arenaModeChanged = true;
                }
                else if (header->type == PacketType::FireEvent && bytesRecv >= static_cast<int>(sizeof(FireEventPacket)))
                {
                    const auto* firePkt = reinterpret_cast<const FireEventPacket*>(buffer);
                    if (weapons)
                    {
                        XMFLOAT3 muzzle = { firePkt->muzzleX, firePkt->muzzleY, firePkt->muzzleZ };
                        XMFLOAT3 target = { firePkt->targetX, firePkt->targetY, firePkt->targetZ };
                        bool isLeft = (firePkt->isLeftArm != 0);
                        weapons->SpawnRemoteProjectile(isLeft, muzzle, target);
                        if (audio)
                        {
                            if (isLeft) audio->PlayShootLeft(muzzle);
                            else audio->PlayShootRight(muzzle);
                        }
                    }
                }
                else if (header->type == PacketType::HitEvent && bytesRecv >= static_cast<int>(sizeof(HitEventPacket)))
                {
                    const auto* hitPkt = reinterpret_cast<const HitEventPacket*>(buffer);
                    if (hitPkt->targetPlayerId == m_localPlayerId && localMech)
                    {
                        XMFLOAT3 hitPos = { hitPkt->hitX, hitPkt->hitY, hitPkt->hitZ };
                        localMech->TakeDamage(hitPkt->damage, hitPkt->impact, hitPkt->directHitMult, &hitPos, audio);
                        if (audio) audio->PlayExplosion(hitPos, 1.25f);
                    }
                    else if (hitPkt->targetPlayerId < MAX_PLAYERS)
                    {
                        m_remoteMechs[hitPkt->targetPlayerId].TakeDamage(hitPkt->damage, hitPkt->impact, hitPkt->directHitMult, audio);
                    }
                }
            }
            // =================================================================
            // Client logic: apply states from host and other players
            // =================================================================
            else if (m_role == NetworkRole::Client)
            {
                m_hostTimeoutTimer = 0.0f;

                if (header->type == PacketType::HandshakeAck && bytesRecv >= static_cast<int>(sizeof(HandshakePacket)))
                {
                    const auto* ack = reinterpret_cast<const HandshakePacket*>(buffer);
                    m_localPlayerId = ack->assignedId;
                    m_state = NetworkState::Connected;
                    m_remoteMechs[0].Respawn({ 0.0f, 2.0f, -40.0f }, 0.0f); // Host is active at North spawn
                    m_remoteMechs[0].SetActive(true);

                    std::cout << "========================================================" << std::endl;
                    std::cout << " [NETWORK] CONNECTED TO 4-PLAYER BATTLE! Assigned Player ID: "
                              << static_cast<int>(m_localPlayerId + 1) << std::endl;
                    std::cout << "========================================================" << std::endl;

                    if (localMech)
                    {
                        XMFLOAT3 spawnPos = { 0.0f, 2.0f, 40.0f };
                        float spawnYaw = 3.14159265f;
                        if (m_localPlayerId == 2) { spawnPos = { 40.0f, 2.0f, 0.0f }; spawnYaw = -1.5707963f; }
                        else if (m_localPlayerId == 3) { spawnPos = { -40.0f, 2.0f, 0.0f }; spawnYaw = 1.5707963f; }
                        localMech->Respawn(spawnPos, spawnYaw);
                    }
                    continue;
                }

                uint8_t senderId = header->senderId;
                if (senderId >= MAX_PLAYERS || senderId == m_localPlayerId) continue;

                if (header->type == PacketType::MechState && bytesRecv >= static_cast<int>(sizeof(MechStatePacket)))
                {
                    const auto* statePkt = reinterpret_cast<const MechStatePacket*>(buffer);
                    m_remoteMechs[senderId].ApplyState(*statePkt);
                }
                else if (header->type == PacketType::ArenaMode && bytesRecv >= static_cast<int>(sizeof(ArenaModePacket)))
                {
                    const auto* arenaPkt = reinterpret_cast<const ArenaModePacket*>(buffer);
                    m_syncedArenaMode = arenaPkt->arenaMode;
                    m_arenaModeChanged = true;
                }
                else if (header->type == PacketType::FireEvent && bytesRecv >= static_cast<int>(sizeof(FireEventPacket)))
                {
                    const auto* firePkt = reinterpret_cast<const FireEventPacket*>(buffer);
                    if (weapons)
                    {
                        XMFLOAT3 muzzle = { firePkt->muzzleX, firePkt->muzzleY, firePkt->muzzleZ };
                        XMFLOAT3 target = { firePkt->targetX, firePkt->targetY, firePkt->targetZ };
                        bool isLeft = (firePkt->isLeftArm != 0);
                        weapons->SpawnRemoteProjectile(isLeft, muzzle, target);
                        if (audio)
                        {
                            if (isLeft) audio->PlayShootLeft(muzzle);
                            else audio->PlayShootRight(muzzle);
                        }
                    }
                }
                else if (header->type == PacketType::HitEvent && bytesRecv >= static_cast<int>(sizeof(HitEventPacket)))
                {
                    const auto* hitPkt = reinterpret_cast<const HitEventPacket*>(buffer);
                    if (hitPkt->targetPlayerId == m_localPlayerId && localMech)
                    {
                        XMFLOAT3 hitPos = { hitPkt->hitX, hitPkt->hitY, hitPkt->hitZ };
                        localMech->TakeDamage(hitPkt->damage, hitPkt->impact, hitPkt->directHitMult, &hitPos, audio);
                        if (audio) audio->PlayExplosion(hitPos, 1.25f);
                    }
                    else if (hitPkt->targetPlayerId < MAX_PLAYERS)
                    {
                        m_remoteMechs[hitPkt->targetPlayerId].TakeDamage(hitPkt->damage, hitPkt->impact, hitPkt->directHitMult, audio);
                    }
                }
            }
        }
    }
}
