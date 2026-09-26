#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <array>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace Overdrive
{
    using namespace DirectX;

    class PhysicsManager;
    class WeaponSystem;
    class AudioManager;
    class MechController;

    enum class NetworkRole
    {
        None,
        Host,
        Client
    };

    enum class NetworkState
    {
        Offline,
        Hosting,        // Waiting for peer connections
        Connecting,     // Trying to connect to host
        Connected,      // Actively synchronized and battling
        Disconnected
    };

    enum class PacketType : uint8_t
    {
        HandshakeReq = 1,
        HandshakeAck = 2,
        MechState    = 3,
        FireEvent    = 4,
        HitEvent     = 5,
        Ping         = 6,
        ArenaMode    = 7
    };

    constexpr uint32_t NET_MAGIC = 0x4F444332; // "ODC2" (Overdrive Core v2 - 4-Player Battle)
    constexpr int MAX_PLAYERS = 4;

#pragma pack(push, 1)
    struct PacketHeader
    {
        uint32_t magic = NET_MAGIC;
        PacketType type;
        uint8_t senderId = 0; // 0 = Host, 1 = Client 1, 2 = Client 2, 3 = Client 3
    };

    struct HandshakePacket
    {
        PacketHeader header;
        uint8_t assignedId = 0; // Host informs client of its assigned Player ID (1, 2, or 3)
    };

    struct MechStatePacket
    {
        PacketHeader header;
        uint32_t sequence;
        float posX, posY, posZ;
        float velX, velY, velZ;
        float yaw, pitch, roll;
        float hp;
        uint8_t flags; // bit0: Boost, bit1: QB, bit2: AB, bit3: Grounded, bit4: Staggered, bit5: BoostKick
    };

    struct FireEventPacket
    {
        PacketHeader header;
        uint8_t isLeftArm; // 0: Right, 1: Left
        float muzzleX, muzzleY, muzzleZ;
        float targetX, targetY, targetZ;
    };

    struct HitEventPacket
    {
        PacketHeader header;
        uint8_t targetPlayerId;
        float damage;
        float hitX, hitY, hitZ;
        float impact = 60.0f;
        float directHitMult = 1.6f;
    };

    struct ArenaModePacket
    {
        PacketHeader header;
        uint8_t arenaMode; // 0: Empty, 1: TargetDummies, 2: CombatBots
    };
#pragma pack(pop)

    // Represents a remote player mech in the 4-player battle arena
    class RemoteMech
    {
    public:
        RemoteMech();
        ~RemoteMech();

        void InitializePhysics(PhysicsManager* physics);
        void Update(float deltaTime, PhysicsManager* physics);

        void ApplyState(const MechStatePacket& packet);
        void TakeDamage(float damage, float impact = 60.0f, float directHitMult = 1.6f, AudioManager* audio = nullptr);
        void ApplyKnockback(const XMFLOAT3& direction, float force);
        void Respawn(const XMFLOAT3& spawnPos, float yaw);

        // Getters for rendering, FCS lock-on, and HUD
        bool IsActive() const { return m_isActive; }
        void SetActive(bool active) { m_isActive = active; }

        uint8_t GetPlayerId() const { return m_playerId; }
        void SetPlayerId(uint8_t id);
        const std::string& GetPlayerName() const { return m_playerName; }

        XMFLOAT4 GetPrimaryColor() const { return m_primaryColor; }
        XMFLOAT4 GetSecondaryColor() const { return m_secondaryColor; }

        XMFLOAT3 GetPosition() const { return m_currentPos; }
        XMFLOAT3 GetVelocity() const { return m_velocity; }
        float GetYaw() const { return m_currentYaw; }
        float GetPitch() const { return m_currentPitch; }
        float GetRoll() const { return m_currentRoll; }
        XMFLOAT3 GetRotation() const { return XMFLOAT3(m_currentYaw, m_currentPitch, m_currentRoll); }

        float GetHp() const { return m_currentHp; }
        float GetMaxHp() const { return m_maxHp; }
        float GetHpRatio() const { return m_maxHp > 0.0f ? (m_currentHp / m_maxHp) : 0.0f; }
        float GetAcs() const { return m_currentAcs; }
        float GetMaxAcs() const { return m_maxAcs; }
        float GetAcsRatio() const { return m_maxAcs > 0.0f ? (m_currentAcs / m_maxAcs) : 0.0f; }
        bool IsStaggered() const { return m_isStaggered; }
        float GetStaggerTimer() const { return m_staggerTimer; }
        bool IsAlive() const { return !m_isDestroyed && m_isActive; }
        bool IsDestroyed() const { return m_isDestroyed; }
        bool IsHitFlashing() const { return m_hitFlashTimer > 0.0f; }

        bool IsBoostMode() const { return m_boostOn; }
        bool IsQuickBoost() const { return m_isQB; }
        bool IsAssaultBoost() const { return m_isAB; }
        bool IsAssaultBoosting() const { return m_isAB; }
        bool IsBoostKicking() const { return m_isBoostKick; }

        JPH::BodyID GetBodyID() const { return m_bodyId; }

    private:
        bool m_isActive = false;
        uint8_t m_playerId = 0;
        std::string m_playerName = "Remote Mech";

        XMFLOAT4 m_primaryColor   = { 0.85f, 0.15f, 0.20f, 1.0f }; // Host red by default
        XMFLOAT4 m_secondaryColor = { 0.20f, 0.22f, 0.25f, 1.0f };

        // Current interpolated transform
        XMFLOAT3 m_currentPos   = { 0.0f, -100.0f, 0.0f };
        XMFLOAT3 m_targetPos    = { 0.0f, -100.0f, 0.0f };
        XMFLOAT3 m_velocity     = { 0.0f, 0.0f, 0.0f };
        float m_currentYaw      = 0.0f;
        float m_targetYaw       = 0.0f;
        float m_currentPitch    = 0.0f;
        float m_targetPitch     = 0.0f;
        float m_currentRoll     = 0.0f;
        float m_targetRoll      = 0.0f;

        // Flags
        bool m_boostOn          = false;
        bool m_isQB             = false;
        bool m_isAB             = false;
        bool m_isBoostKick      = false;
        bool m_isGrounded       = true;

        // Health & Combat
        float m_currentHp       = 2500.0f;
        const float m_maxHp     = 2500.0f;
        bool m_isDestroyed      = false;
        float m_hitFlashTimer   = 0.0f;
        float m_respawnTimer    = 0.0f;
        bool m_hasReceivedFirstPacket = false;

        // ACS & Stagger
        float m_currentAcs      = 0.0f;
        float m_maxAcs          = 1200.0f;
        bool  m_isStaggered     = false;
        float m_staggerTimer    = 0.0f;
        float m_acsCooldown     = 0.0f;
        const float c_staggerDuration = 2.4f;

        // Jolt physics collider for raycast bullet impacts
        JPH::BodyID m_bodyId;
    };

    struct ClientSlot
    {
        bool active = false;
        sockaddr_in addr = {};
        uint8_t playerId = 0;
        float timeoutTimer = 0.0f;
    };

    class NetworkManager
    {
    public:
        NetworkManager();
        ~NetworkManager();

        bool Initialize();
        void Shutdown();

        // Start as Host (listens on specified port)
        bool StartHost(uint16_t port = 7777);

        // Start as Client (connects to hostIp:port)
        bool StartClient(const std::string& hostIp, uint16_t port = 7777);

        // Convenient update using MechController directly
        void Update(
            float deltaTime,
            MechController& localMech,
            WeaponSystem* weapons = nullptr,
            AudioManager* audio = nullptr,
            PhysicsManager* physics = nullptr
        );

        // Periodic update (send local state, receive remote packets, update RemoteMechs)
        void Update(
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
            WeaponSystem* weapons = nullptr,
            AudioManager* audio = nullptr,
            PhysicsManager* physics = nullptr,
            MechController* localMech = nullptr,
            bool isStaggered = false,
            bool isBoostKick = false
        );

        // Notify peers that local player fired weapon
        void SendFireEvent(bool isLeftArm, const XMFLOAT3& muzzlePos, const XMFLOAT3& targetPos);

        // Notify peers that bullet hit a target player
        void SendHitEvent(uint8_t targetPlayerId, float damage, const XMFLOAT3& hitPos, float impact = 60.0f, float directHitMult = 1.6f);

        // Arena mode synchronization (0: Empty, 1: TargetDummies, 2: CombatBots)
        void BroadcastArenaMode(uint8_t mode);
        uint8_t GetSyncedArenaMode() const { return m_syncedArenaMode; }
        void SetSyncedArenaMode(uint8_t mode) { m_syncedArenaMode = mode; }
        bool ConsumeArenaModeChanged() { bool c = m_arenaModeChanged; m_arenaModeChanged = false; return c; }

        // Status & Remote Mechs
        NetworkState GetState() const { return m_state; }
        NetworkRole GetRole() const { return m_role; }
        bool IsHost() const { return m_role == NetworkRole::Host; }
        bool IsClient() const { return m_role == NetworkRole::Client; }
        NetworkRole GetMode() const { return m_role; }
        bool IsConnected() const { return m_state == NetworkState::Connected; }
        std::string GetStatusString() const;

        uint8_t GetLocalPlayerId() const { return m_localPlayerId; }
        int GetConnectedPlayerCount() const;

        // Remote mech access for rendering and targeting
        std::vector<RemoteMech>& GetRemoteMechs() { return m_remoteMechs; }
        const std::vector<RemoteMech>& GetRemoteMechs() const { return m_remoteMechs; }

        // Backward compatibility: returns first active remote mech
        RemoteMech& GetRemoteMech();
        const RemoteMech& GetRemoteMech() const;

        void InitializeRemotePhysics(PhysicsManager* physics);

    private:
        void ProcessIncomingPackets(WeaponSystem* weapons, AudioManager* audio, PhysicsManager* physics, MechController* localMech);
        void SendStatePacket(
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
            bool isStaggered = false,
            bool isBoostKick = false
        );

        void RelayPacket(const void* data, int size, const sockaddr_in& senderAddr);
        void SendPacketTo(const void* data, int size, const sockaddr_in& targetAddr);

    private:
        bool m_initialized = false;
        SOCKET m_socket = INVALID_SOCKET;
        NetworkRole m_role = NetworkRole::None;
        NetworkState m_state = NetworkState::Offline;

        uint8_t m_localPlayerId = 0; // 0 for host, 1-3 for clients

        // Host: connected clients
        std::array<ClientSlot, MAX_PLAYERS - 1> m_clientSlots;

        // Client: host destination address
        sockaddr_in m_hostAddr = {};
        bool m_hasHostAddr = false;

        std::string m_targetIp = "127.0.0.1";
        uint16_t m_port = 7777;

        uint32_t m_sendSequence = 0;
        uint32_t m_recvSequence = 0;

        float m_heartbeatTimer = 0.0f;
        float m_hostTimeoutTimer = 0.0f;
        const float c_timeoutDuration = 4.0f;

        uint8_t m_syncedArenaMode = 1; // Default: TargetDummies
        bool m_arenaModeChanged = false;

        // Remote mechs (one for each other player slot, max 3)
        std::vector<RemoteMech> m_remoteMechs;
    };
}
