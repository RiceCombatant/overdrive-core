#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <DirectXMath.h>
#include <string>
#include <vector>
#include <memory>

// Forward declarations for miniaudio structures
struct ma_engine;
struct ma_sound;

namespace Overdrive
{
    using namespace DirectX;

    class AudioManager
    {
    public:
        AudioManager();
        ~AudioManager();

        bool Initialize();
        void Shutdown();

        // 3D Audio Listener update (camera eye pos and forward direction)
        void UpdateListener(const XMFLOAT3& eyePos, const XMFLOAT3& forward, const XMFLOAT3& up);

        // Sound triggers
        void PlayShootRight(const XMFLOAT3& muzzlePos);
        void PlayShootLeft(const XMFLOAT3& muzzlePos);
        void PlayQuickBoost(const XMFLOAT3& mechPos);
        void PlayExplosion(const XMFLOAT3& hitPos, float volume = 1.0f);
        void PlayLockOn(); // 2D Cockpit UI alert sound

        // Continuous Thruster Loop
        void UpdateBoostSound(bool isBoosting, float speedRatio, const XMFLOAT3& mechPos);

    private:
        bool EnsureAudioAssetsExist();
        void GenerateProceduralWavFiles();

        std::unique_ptr<ma_engine> m_engine;

        // Sound pools for polyphonic playback
        static const int c_poolSize = 4;
        std::vector<std::unique_ptr<ma_sound>> m_shootRightSounds;
        std::vector<std::unique_ptr<ma_sound>> m_shootLeftSounds;
        std::vector<std::unique_ptr<ma_sound>> m_qbSounds;
        std::vector<std::unique_ptr<ma_sound>> m_explosionSounds;
        std::vector<std::unique_ptr<ma_sound>> m_lockOnSounds;

        int m_shootRightIdx = 0;
        int m_shootLeftIdx  = 0;
        int m_qbIdx         = 0;
        int m_explosionIdx  = 0;
        int m_lockOnIdx     = 0;

        // Looping thruster sound
        std::unique_ptr<ma_sound> m_boostLoopSound;
        bool m_isBoostLoopPlaying = false;
        bool m_initialized = false;
    };
}
