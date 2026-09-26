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
        void PlayShootBazooka(const XMFLOAT3& pos);
        void PlayShootMissile(const XMFLOAT3& pos);
        void PlayShootGatling(const XMFLOAT3& pos);
        void PlayBladeSlash(const XMFLOAT3& pos);
        void PlayShootShotgun(const XMFLOAT3& pos);
        void PlayEquipChange();
        void PlayQuickBoost(const XMFLOAT3& mechPos);
        void PlayExplosion(const XMFLOAT3& hitPos, float volume = 1.0f);
        void PlayLockOn(); // 2D Cockpit UI alert sound
        void PlayReload(const XMFLOAT3& pos); // Weapon reload complete sound
        void PlayWeaponSwap(const XMFLOAT3& pos); // Hanger weapon swap mechanical sound
        void PlayMenuMove(); // 2D Menu navigation cursor move sound
        void PlayMenuConfirm(); // 2D Menu selection confirm sound
        void PlayStaggerBreak(const XMFLOAT3& pos); // Heavy impact crash & electrical overload burst
        void PlayStaggerAlarm(); // Critical cockpit overload alarm
        void PlaySystemRestored(); // System restored recovery chime
        void PlayBoostKickThrust(const XMFLOAT3& pos); // High-thrust rocket acceleration impulse
        void PlayBoostKickHit(const XMFLOAT3& pos); // Megaton impact crushing slam sound

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
        std::vector<std::unique_ptr<ma_sound>> m_bazookaSounds;
        std::vector<std::unique_ptr<ma_sound>> m_missileSounds;
        std::vector<std::unique_ptr<ma_sound>> m_gatlingSounds;
        std::vector<std::unique_ptr<ma_sound>> m_bladeSounds;
        std::vector<std::unique_ptr<ma_sound>> m_shotgunSounds;
        std::vector<std::unique_ptr<ma_sound>> m_equipChangeSounds;
        std::vector<std::unique_ptr<ma_sound>> m_qbSounds;
        std::vector<std::unique_ptr<ma_sound>> m_explosionSounds;
        std::vector<std::unique_ptr<ma_sound>> m_lockOnSounds;
        std::vector<std::unique_ptr<ma_sound>> m_reloadSounds;
        std::vector<std::unique_ptr<ma_sound>> m_weaponSwapSounds;
        std::vector<std::unique_ptr<ma_sound>> m_menuMoveSounds;
        std::vector<std::unique_ptr<ma_sound>> m_menuConfirmSounds;
        std::vector<std::unique_ptr<ma_sound>> m_staggerBreakSounds;
        std::vector<std::unique_ptr<ma_sound>> m_staggerAlarmSounds;
        std::vector<std::unique_ptr<ma_sound>> m_systemRestoredSounds;
        std::vector<std::unique_ptr<ma_sound>> m_boostKickThrustSounds;
        std::vector<std::unique_ptr<ma_sound>> m_boostKickHitSounds;

        int m_shootRightIdx     = 0;
        int m_shootLeftIdx      = 0;
        int m_bazookaIdx        = 0;
        int m_missileIdx        = 0;
        int m_gatlingIdx        = 0;
        int m_bladeIdx          = 0;
        int m_shotgunIdx        = 0;
        int m_equipChangeIdx    = 0;
        int m_qbIdx             = 0;
        int m_explosionIdx      = 0;
        int m_lockOnIdx         = 0;
        int m_reloadIdx         = 0;
        int m_weaponSwapIdx     = 0;
        int m_menuMoveIdx       = 0;
        int m_menuConfirmIdx    = 0;
        int m_staggerBreakIdx   = 0;
        int m_staggerAlarmIdx   = 0;
        int m_systemRestoredIdx = 0;
        int m_boostKickThrustIdx= 0;
        int m_boostKickHitIdx   = 0;

        // Looping thruster sound
        std::unique_ptr<ma_sound> m_boostLoopSound;
        bool m_isBoostLoopPlaying = false;
        bool m_initialized = false;
    };
}
