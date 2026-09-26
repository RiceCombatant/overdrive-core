#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "audio/AudioManager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <random>

namespace Overdrive
{
    namespace
    {
        // Simple 16-bit Mono WAV writer
        bool SaveWavFile(const std::string& filepath, const std::vector<int16_t>& samples, uint32_t sampleRate = 44100)
        {
            std::ofstream out(filepath, std::ios::binary);
            if (!out.is_open()) return false;

            uint32_t numSamples = static_cast<uint32_t>(samples.size());
            uint32_t byteRate = sampleRate * sizeof(int16_t);
            uint32_t dataChunkSize = numSamples * sizeof(int16_t);
            uint32_t totalFileSize = 36 + dataChunkSize;

            // RIFF header
            out.write("RIFF", 4);
            out.write(reinterpret_cast<const char*>(&totalFileSize), 4);
            out.write("WAVE", 4);

            // fmt subchunk
            out.write("fmt ", 4);
            uint32_t subchunk1Size = 16;
            uint16_t audioFormat = 1; // PCM
            uint16_t numChannels = 1; // Mono
            uint16_t blockAlign = sizeof(int16_t);
            uint16_t bitsPerSample = 16;
            out.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
            out.write(reinterpret_cast<const char*>(&audioFormat), 2);
            out.write(reinterpret_cast<const char*>(&numChannels), 2);
            out.write(reinterpret_cast<const char*>(&sampleRate), 4);
            out.write(reinterpret_cast<const char*>(&byteRate), 4);
            out.write(reinterpret_cast<const char*>(&blockAlign), 2);
            out.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

            // data subchunk
            out.write("data", 4);
            out.write(reinterpret_cast<const char*>(&dataChunkSize), 4);
            out.write(reinterpret_cast<const char*>(samples.data()), dataChunkSize);

            return true;
        }
    }

    AudioManager::AudioManager()
    {
    }

    AudioManager::~AudioManager()
    {
        Shutdown();
    }

    bool AudioManager::Initialize()
    {
        // 1. Ensure directory and procedural sound files exist
        if (!EnsureAudioAssetsExist())
        {
            std::cerr << "[AUDIO] Failed to initialize audio asset files" << std::endl;
        }

        // 2. Initialize miniaudio engine with 3D spatial settings
        m_engine = std::make_unique<ma_engine>();
        ma_engine_config engineConfig = ma_engine_config_init();
        engineConfig.listenerCount = 1;

        ma_result result = ma_engine_init(&engineConfig, m_engine.get());
        if (result != MA_SUCCESS)
        {
            std::cerr << "[AUDIO] Failed to initialize miniaudio engine: " << result << std::endl;
            m_engine.reset();
            return false;
        }

        // Set comfortable master volume (55%) to avoid ear fatigue
        ma_engine_set_volume(m_engine.get(), 0.55f);

        // 3. Load sound pools
        auto loadSoundPool = [&](const std::string& path, std::vector<std::unique_ptr<ma_sound>>& pool, bool spatial) {
            pool.resize(c_poolSize);
            for (int i = 0; i < c_poolSize; ++i)
            {
                pool[i] = std::make_unique<ma_sound>();
                ma_uint32 flags = spatial ? MA_SOUND_FLAG_DECODE : (MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION);
                ma_result res = ma_sound_init_from_file(m_engine.get(), path.c_str(), flags, nullptr, nullptr, pool[i].get());
                if (res == MA_SUCCESS && spatial)
                {
                    ma_sound_set_min_distance(pool[i].get(), 4.0f);
                    ma_sound_set_max_distance(pool[i].get(), 280.0f);
                    ma_sound_set_attenuation_model(pool[i].get(), ma_attenuation_model_inverse);
                }
            }
        };

        loadSoundPool("assets/audio/shoot_right.wav", m_shootRightSounds, true);
        loadSoundPool("assets/audio/shoot_left.wav",  m_shootLeftSounds,  true);
        loadSoundPool("assets/audio/shoot_bazooka.wav", m_bazookaSounds, true);
        loadSoundPool("assets/audio/shoot_missile.wav", m_missileSounds, true);
        loadSoundPool("assets/audio/shoot_gatling.wav", m_gatlingSounds, true);
        loadSoundPool("assets/audio/blade_slash.wav",   m_bladeSounds,   true);
        loadSoundPool("assets/audio/shoot_shotgun.wav", m_shotgunSounds, true);
        loadSoundPool("assets/audio/equip_change.wav",  m_equipChangeSounds, false);
        loadSoundPool("assets/audio/quick_boost.wav", m_qbSounds,         true);
        loadSoundPool("assets/audio/explosion.wav",   m_explosionSounds,  true);
        loadSoundPool("assets/audio/lock_on.wav",     m_lockOnSounds,     false); // 2D cockpit sound
        loadSoundPool("assets/audio/reload.wav",      m_reloadSounds,      true);
        loadSoundPool("assets/audio/weapon_swap.wav", m_weaponSwapSounds,  true);
        loadSoundPool("assets/audio/menu_move.wav",   m_menuMoveSounds,    false); // 2D UI sound
        loadSoundPool("assets/audio/menu_confirm.wav",m_menuConfirmSounds, false); // 2D UI sound
        loadSoundPool("assets/audio/stagger_break.wav", m_staggerBreakSounds, true);
        loadSoundPool("assets/audio/stagger_alarm.wav", m_staggerAlarmSounds, false); // 2D cockpit alarm
        loadSoundPool("assets/audio/system_restored.wav", m_systemRestoredSounds, false); // 2D chime
        loadSoundPool("assets/audio/boost_kick_thrust.wav", m_boostKickThrustSounds, true);
        loadSoundPool("assets/audio/boost_kick_hit.wav",    m_boostKickHitSounds,    true);

        // 4. Load looping thruster sound
        m_boostLoopSound = std::make_unique<ma_sound>();
        ma_result loopRes = ma_sound_init_from_file(
            m_engine.get(),
            "assets/audio/boost_loop.wav",
            MA_SOUND_FLAG_DECODE,
            nullptr,
            nullptr,
            m_boostLoopSound.get()
        );
        if (loopRes == MA_SUCCESS)
        {
            ma_sound_set_looping(m_boostLoopSound.get(), MA_TRUE);
            ma_sound_set_volume(m_boostLoopSound.get(), 0.0f);
            ma_sound_set_min_distance(m_boostLoopSound.get(), 3.0f);
            ma_sound_set_max_distance(m_boostLoopSound.get(), 60.0f);
            ma_sound_start(m_boostLoopSound.get());
            m_isBoostLoopPlaying = true;
        }

        m_initialized = true;
        std::cout << "[AUDIO] miniaudio 3D Spatial Audio System initialized successfully!" << std::endl;
        return true;
    }

    void AudioManager::Shutdown()
    {
        if (m_boostLoopSound)
        {
            ma_sound_uninit(m_boostLoopSound.get());
            m_boostLoopSound.reset();
        }

        auto cleanupPool = [](std::vector<std::unique_ptr<ma_sound>>& pool) {
            for (auto& s : pool)
            {
                if (s) ma_sound_uninit(s.get());
            }
            pool.clear();
        };

        cleanupPool(m_shootRightSounds);
        cleanupPool(m_shootLeftSounds);
        cleanupPool(m_bazookaSounds);
        cleanupPool(m_missileSounds);
        cleanupPool(m_gatlingSounds);
        cleanupPool(m_bladeSounds);
        cleanupPool(m_shotgunSounds);
        cleanupPool(m_equipChangeSounds);
        cleanupPool(m_qbSounds);
        cleanupPool(m_explosionSounds);
        cleanupPool(m_lockOnSounds);
        cleanupPool(m_reloadSounds);
        cleanupPool(m_weaponSwapSounds);
        cleanupPool(m_menuMoveSounds);
        cleanupPool(m_menuConfirmSounds);
        cleanupPool(m_staggerBreakSounds);
        cleanupPool(m_staggerAlarmSounds);
        cleanupPool(m_systemRestoredSounds);
        cleanupPool(m_boostKickThrustSounds);
        cleanupPool(m_boostKickHitSounds);

        if (m_engine)
        {
            ma_engine_uninit(m_engine.get());
            m_engine.reset();
        }
        m_initialized = false;
    }

    void AudioManager::UpdateListener(const XMFLOAT3& eyePos, const XMFLOAT3& forward, const XMFLOAT3& up)
    {
        if (!m_initialized || !m_engine) return;

        ma_engine_listener_set_position(m_engine.get(), 0, eyePos.x, eyePos.y, eyePos.z);
        ma_engine_listener_set_direction(m_engine.get(), 0, forward.x, forward.y, forward.z);
        ma_engine_listener_set_world_up(m_engine.get(), 0, up.x, up.y, up.z);
    }

    void AudioManager::PlayShootRight(const XMFLOAT3& muzzlePos)
    {
        if (!m_initialized || m_shootRightSounds.empty()) return;
        auto& sound = m_shootRightSounds[m_shootRightIdx];
        m_shootRightIdx = (m_shootRightIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), muzzlePos.x, muzzlePos.y, muzzlePos.z);
        ma_sound_set_volume(sound.get(), 0.55f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayShootLeft(const XMFLOAT3& muzzlePos)
    {
        if (!m_initialized || m_shootLeftSounds.empty()) return;
        auto& sound = m_shootLeftSounds[m_shootLeftIdx];
        m_shootLeftIdx = (m_shootLeftIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), muzzlePos.x, muzzlePos.y, muzzlePos.z);
        ma_sound_set_volume(sound.get(), 0.50f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayShootBazooka(const XMFLOAT3& pos)
    {
        if (!m_initialized || m_bazookaSounds.empty()) return;
        auto& sound = m_bazookaSounds[m_bazookaIdx];
        m_bazookaIdx = (m_bazookaIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), pos.x, pos.y, pos.z);
        ma_sound_set_volume(sound.get(), 0.85f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayShootMissile(const XMFLOAT3& pos)
    {
        if (!m_initialized || m_missileSounds.empty()) return;
        auto& sound = m_missileSounds[m_missileIdx];
        m_missileIdx = (m_missileIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), pos.x, pos.y, pos.z);
        ma_sound_set_volume(sound.get(), 0.65f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayShootGatling(const XMFLOAT3& pos)
    {
        if (!m_initialized || m_gatlingSounds.empty()) return;
        auto& sound = m_gatlingSounds[m_gatlingIdx];
        m_gatlingIdx = (m_gatlingIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), pos.x, pos.y, pos.z);
        ma_sound_set_volume(sound.get(), 0.45f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayBladeSlash(const XMFLOAT3& pos)
    {
        if (!m_initialized || m_bladeSounds.empty()) return;
        auto& sound = m_bladeSounds[m_bladeIdx];
        m_bladeIdx = (m_bladeIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), pos.x, pos.y, pos.z);
        ma_sound_set_volume(sound.get(), 0.75f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayShootShotgun(const XMFLOAT3& pos)
    {
        if (!m_initialized || m_shotgunSounds.empty()) return;
        auto& sound = m_shotgunSounds[m_shotgunIdx];
        m_shotgunIdx = (m_shotgunIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), pos.x, pos.y, pos.z);
        ma_sound_set_volume(sound.get(), 0.70f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayEquipChange()
    {
        if (!m_initialized || m_equipChangeSounds.empty()) return;
        auto& sound = m_equipChangeSounds[m_equipChangeIdx];
        m_equipChangeIdx = (m_equipChangeIdx + 1) % c_poolSize;

        ma_sound_set_volume(sound.get(), 0.65f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayQuickBoost(const XMFLOAT3& mechPos)
    {
        if (!m_initialized || m_qbSounds.empty()) return;
        auto& sound = m_qbSounds[m_qbIdx];
        m_qbIdx = (m_qbIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), mechPos.x, mechPos.y, mechPos.z);
        ma_sound_set_volume(sound.get(), 0.70f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayExplosion(const XMFLOAT3& hitPos, float volume)
    {
        if (!m_initialized || m_explosionSounds.empty()) return;
        auto& sound = m_explosionSounds[m_explosionIdx];
        m_explosionIdx = (m_explosionIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), hitPos.x, hitPos.y, hitPos.z);
        ma_sound_set_volume(sound.get(), std::clamp(volume * 0.60f, 0.1f, 0.75f));
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayLockOn()
    {
        if (!m_initialized || m_lockOnSounds.empty()) return;
        auto& sound = m_lockOnSounds[m_lockOnIdx];
        m_lockOnIdx = (m_lockOnIdx + 1) % c_poolSize;

        ma_sound_set_volume(sound.get(), 0.40f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayReload(const XMFLOAT3& pos)
    {
        if (!m_initialized || m_reloadSounds.empty()) return;
        auto& sound = m_reloadSounds[m_reloadIdx];
        m_reloadIdx = (m_reloadIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), pos.x, pos.y, pos.z);
        ma_sound_set_volume(sound.get(), 0.65f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayWeaponSwap(const XMFLOAT3& pos)
    {
        if (!m_initialized || m_weaponSwapSounds.empty()) return;
        auto& sound = m_weaponSwapSounds[m_weaponSwapIdx];
        m_weaponSwapIdx = (m_weaponSwapIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), pos.x, pos.y, pos.z);
        ma_sound_set_volume(sound.get(), 0.68f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayMenuMove()
    {
        if (!m_initialized || m_menuMoveSounds.empty()) return;
        auto& sound = m_menuMoveSounds[m_menuMoveIdx];
        m_menuMoveIdx = (m_menuMoveIdx + 1) % c_poolSize;

        ma_sound_set_volume(sound.get(), 0.35f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayMenuConfirm()
    {
        if (!m_initialized || m_menuConfirmSounds.empty()) return;
        auto& sound = m_menuConfirmSounds[m_menuConfirmIdx];
        m_menuConfirmIdx = (m_menuConfirmIdx + 1) % c_poolSize;

        ma_sound_set_volume(sound.get(), 0.55f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayStaggerBreak(const XMFLOAT3& pos)
    {
        if (!m_initialized || m_staggerBreakSounds.empty()) return;
        auto& sound = m_staggerBreakSounds[m_staggerBreakIdx];
        m_staggerBreakIdx = (m_staggerBreakIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), pos.x, pos.y, pos.z);
        ma_sound_set_volume(sound.get(), 0.85f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayStaggerAlarm()
    {
        if (!m_initialized || m_staggerAlarmSounds.empty()) return;
        auto& sound = m_staggerAlarmSounds[m_staggerAlarmIdx];
        m_staggerAlarmIdx = (m_staggerAlarmIdx + 1) % c_poolSize;

        ma_sound_set_volume(sound.get(), 0.65f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlaySystemRestored()
    {
        if (!m_initialized || m_systemRestoredSounds.empty()) return;
        auto& sound = m_systemRestoredSounds[m_systemRestoredIdx];
        m_systemRestoredIdx = (m_systemRestoredIdx + 1) % c_poolSize;

        ma_sound_set_volume(sound.get(), 0.60f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayBoostKickThrust(const XMFLOAT3& pos)
    {
        if (!m_initialized || m_boostKickThrustSounds.empty()) return;
        auto& sound = m_boostKickThrustSounds[m_boostKickThrustIdx];
        m_boostKickThrustIdx = (m_boostKickThrustIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), pos.x, pos.y, pos.z);
        ma_sound_set_volume(sound.get(), 0.90f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::PlayBoostKickHit(const XMFLOAT3& pos)
    {
        if (!m_initialized || m_boostKickHitSounds.empty()) return;
        auto& sound = m_boostKickHitSounds[m_boostKickHitIdx];
        m_boostKickHitIdx = (m_boostKickHitIdx + 1) % c_poolSize;

        ma_sound_set_position(sound.get(), pos.x, pos.y, pos.z);
        ma_sound_set_volume(sound.get(), 1.0f);
        ma_sound_seek_to_pcm_frame(sound.get(), 0);
        ma_sound_start(sound.get());
    }

    void AudioManager::UpdateBoostSound(bool isBoosting, float speedRatio, const XMFLOAT3& mechPos)
    {
        if (!m_initialized || !m_boostLoopSound) return;

        ma_sound_set_position(m_boostLoopSound.get(), mechPos.x, mechPos.y, mechPos.z);

        // Completely silent when stationary or not moving
        float targetVol = (isBoosting && speedRatio > 0.08f) ? std::clamp(speedRatio * 0.15f, 0.02f, 0.15f) : 0.0f;
        float currentVol = ma_sound_get_volume(m_boostLoopSound.get());
        float newVol = currentVol + (targetVol - currentVol) * 0.15f;
        if (newVol < 0.005f) newVol = 0.0f;
        ma_sound_set_volume(m_boostLoopSound.get(), newVol);
        ma_sound_set_pitch(m_boostLoopSound.get(), 0.90f + speedRatio * 0.25f);
    }

    bool AudioManager::EnsureAudioAssetsExist()
    {
        std::filesystem::create_directories("assets/audio");

        bool shootR = std::filesystem::exists("assets/audio/shoot_right.wav");
        bool shootL = std::filesystem::exists("assets/audio/shoot_left.wav");
        bool bazooka= std::filesystem::exists("assets/audio/shoot_bazooka.wav");
        bool missile= std::filesystem::exists("assets/audio/shoot_missile.wav");
        bool gatling= std::filesystem::exists("assets/audio/shoot_gatling.wav");
        bool blade  = std::filesystem::exists("assets/audio/blade_slash.wav");
        bool shotgun= std::filesystem::exists("assets/audio/shoot_shotgun.wav");
        bool eqChg  = std::filesystem::exists("assets/audio/equip_change.wav");
        bool qb     = std::filesystem::exists("assets/audio/quick_boost.wav");
        bool explo  = std::filesystem::exists("assets/audio/explosion.wav");
        bool lock   = std::filesystem::exists("assets/audio/lock_on.wav");
        bool bLoop  = std::filesystem::exists("assets/audio/boost_loop.wav");
        bool reload = std::filesystem::exists("assets/audio/reload.wav");
        bool wSwap  = std::filesystem::exists("assets/audio/weapon_swap.wav");
        bool mMove  = std::filesystem::exists("assets/audio/menu_move.wav");
        bool mConf  = std::filesystem::exists("assets/audio/menu_confirm.wav");
        bool sBreak = std::filesystem::exists("assets/audio/stagger_break.wav");
        bool sAlarm = std::filesystem::exists("assets/audio/stagger_alarm.wav");
        bool sRest  = std::filesystem::exists("assets/audio/system_restored.wav");
        bool bKickT = std::filesystem::exists("assets/audio/boost_kick_thrust.wav");
        bool bKickH = std::filesystem::exists("assets/audio/boost_kick_hit.wav");

        if (!shootR || !shootL || !bazooka || !missile || !gatling || !blade || !shotgun || !eqChg ||
            !qb || !explo || !lock || !bLoop || !reload || !wSwap || !mMove || !mConf ||
            !sBreak || !sAlarm || !sRest || !bKickT || !bKickH)
        {
            std::cout << "[AUDIO] Generating procedural sci-fi audio effects..." << std::endl;
            GenerateProceduralWavFiles();
        }
        return true;
    }

    void AudioManager::GenerateProceduralWavFiles()
    {
        const int sampleRate = 44100;
        std::mt19937 rng(1337);
        std::uniform_real_distribution<float> noiseDist(-1.0f, 1.0f);

        // 1. Right Rifle: Punchy kinetic high-velocity round
        {
            float duration = 0.28f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float phase = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float freq = 1400.0f * std::exp(-22.0f * t) + 110.0f;
                phase += XM_2PI * freq / sampleRate;

                float sine = std::sin(phase);
                float noise = noiseDist(rng) * (t < 0.035f ? 1.0f : 0.15f);
                float env = std::exp(-13.0f * t);

                float sample = (sine * 0.75f + noise * 0.25f) * env;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.90f);
            }
            SaveWavFile("assets/audio/shoot_right.wav", samples, sampleRate);
        }

        // 2. Left Rifle: Sharp FM Energy Beam Pulse
        {
            float duration = 0.24f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float phaseC = 0.0f;
            float phaseM = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float carrierFreq = 950.0f * std::exp(-14.0f * t) + 240.0f;
                float modFreq = 320.0f;
                float modIndex = 3.0f * std::exp(-16.0f * t);

                phaseM += XM_2PI * modFreq / sampleRate;
                float modVal = std::sin(phaseM) * modIndex;

                phaseC += XM_2PI * carrierFreq / sampleRate;
                float sample = std::sin(phaseC + modVal) * std::exp(-11.5f * t);

                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.88f);
            }
            SaveWavFile("assets/audio/shoot_left.wav", samples, sampleRate);
        }

        // 3. Quick Boost: Massive low-end thruster explosion + compression
        {
            float duration = 0.44f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float phase = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float attack = std::min(1.0f, t / 0.018f);
                float decay = std::exp(-6.5f * t);

                // Sub-bass thud
                float bassFreq = 95.0f * std::exp(-8.0f * t) + 32.0f;
                phase += XM_2PI * bassFreq / sampleRate;
                float bass = std::sin(phase);

                // Jet exhaust hiss
                float hiss = noiseDist(rng) * std::exp(-7.5f * t);

                float sample = (bass * 0.70f + hiss * 0.30f) * attack * decay;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.95f);
            }
            SaveWavFile("assets/audio/quick_boost.wav", samples, sampleRate);
        }

        // 4. Explosion / Impact: Heavy metal crash + detonation rumble
        {
            float duration = 0.65f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float rumblePhase = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float attack = std::min(1.0f, t / 0.005f);
                float decay = std::exp(-4.2f * t);

                // Low frequency rumble
                rumblePhase += XM_2PI * (60.0f * std::exp(-2.0f * t) + 25.0f) / sampleRate;
                float rumble = std::sin(rumblePhase);

                // Distortion clipped noise
                float rawNoise = noiseDist(rng);
                float clippedNoise = std::clamp(rawNoise * 2.2f, -1.0f, 1.0f);

                float sample = (rumble * 0.45f + clippedNoise * 0.55f) * attack * decay;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.95f);
            }
            SaveWavFile("assets/audio/explosion.wav", samples, sampleRate);
        }

        // 5. Lock-On: Dual futuristic cockpit electronic beep (AC6 style)
        {
            float duration = 0.14f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples, 0);
            float phase1 = 0.0f;
            float phase2 = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float sample = 0.0f;

                if (t >= 0.0f && t < 0.045f)
                {
                    phase1 += XM_2PI * 2300.0f / sampleRate;
                    sample = std::sin(phase1) * (1.0f - t / 0.045f);
                }
                else if (t >= 0.065f && t < 0.125f)
                {
                    float t2 = t - 0.065f;
                    phase2 += XM_2PI * 3100.0f / sampleRate;
                    sample = std::sin(phase2) * (1.0f - t2 / 0.060f);
                }

                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.65f);
            }
            SaveWavFile("assets/audio/lock_on.wav", samples, sampleRate);
        }

        // 6. Boost Loop: Smooth, warm sci-fi thruster hum (NO harsh white noise)
        {
            float duration = 1.0f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                // Pure deep thruster harmonics (65Hz fundamental, 130Hz octave, 195Hz warm overtone)
                p1 += XM_2PI * 65.0f / sampleRate;
                p2 += XM_2PI * 130.0f / sampleRate;
                p3 += XM_2PI * 195.0f / sampleRate;

                float hum = std::sin(p1) * 0.55f + std::sin(p2) * 0.30f + std::sin(p3) * 0.15f;
                // Soft compression to avoid clipping
                float sample = std::tanh(hum) * 0.35f;

                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.25f);
            }
            SaveWavFile("assets/audio/boost_loop.wav", samples, sampleRate);
        }

        // 7. Reload Complete: Mechanical latch click + Dual-tone sci-fi charge chime
        {
            float duration = 0.22f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples, 0);
            float pClick = 0.0f;
            float pChime1 = 0.0f;
            float pChime2 = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float sample = 0.0f;

                // 0.00s - 0.05s: Mechanical click/latch (metallic frequency drop + noise)
                if (t < 0.05f)
                {
                    float clickFreq = 750.0f * std::exp(-50.0f * t) + 120.0f;
                    pClick += XM_2PI * clickFreq / sampleRate;
                    float clickEnv = std::exp(-45.0f * t);
                    sample += (std::sin(pClick) * 0.6f + noiseDist(rng) * 0.4f) * clickEnv * 0.7f;
                }

                // 0.05s - 0.12s: First chime tone (1760Hz, A6)
                if (t >= 0.05f && t < 0.12f)
                {
                    float tRel = t - 0.05f;
                    pChime1 += XM_2PI * 1760.0f / sampleRate;
                    sample += std::sin(pChime1) * std::exp(-22.0f * tRel) * 0.45f;
                }

                // 0.11s - 0.22s: Second higher chime tone (2637Hz, E7) - confirming load
                if (t >= 0.11f)
                {
                    float tRel = t - 0.11f;
                    pChime2 += XM_2PI * 2637.0f / sampleRate;
                    sample += std::sin(pChime2) * std::exp(-20.0f * tRel) * 0.55f;
                }

                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.75f);
            }
            SaveWavFile("assets/audio/reload.wav", samples, sampleRate);
        }

        // 8. Menu Move: Crisp, lightweight UI selection blip (2200Hz -> 1400Hz)
        {
            float duration = 0.045f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples, 0);
            float phase = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float freq = 2200.0f * std::exp(-25.0f * t) + 1200.0f;
                phase += XM_2PI * freq / sampleRate;
                float env = 1.0f - (t / duration);
                float sample = std::sin(phase) * env * 0.40f;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f);
            }
            SaveWavFile("assets/audio/menu_move.wav", samples, sampleRate);
        }

        // 9. Menu Confirm: High-tech positive activation tone (dual-tone chord + metallic strike)
        {
            float duration = 0.18f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples, 0);
            float phase1 = 0.0f;
            float phase2 = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                phase1 += XM_2PI * 1320.0f / sampleRate; // E6
                phase2 += XM_2PI * 1975.0f / sampleRate; // B6
                float env = std::exp(-18.0f * t);
                float sample = (std::sin(phase1) * 0.45f + std::sin(phase2) * 0.55f) * env * 0.65f;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f);
            }
            SaveWavFile("assets/audio/menu_confirm.wav", samples, sampleRate);
        }

        // 10. Weapon Swap: AC-style mechanical hanger release, rack movement, and weapon lock clank
        {
            float duration = 0.32f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples, 0);

            float pRelease = 0.0f;
            float pServo = 0.0f;
            float pClank1 = 0.0f;
            float pClank2 = 0.0f;
            float pBeep = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float sample = 0.0f;

                // 0.00s - 0.06s: Latch release click (high pitch metal snap)
                if (t < 0.06f)
                {
                    pRelease += XM_2PI * (2400.0f - 18000.0f * t) / sampleRate;
                    float noise = noiseDist(rng) * 0.35f;
                    sample += (std::sin(pRelease) * 0.45f + noise) * std::exp(-50.0f * t);
                }

                // 0.04s - 0.18s: Motorized hanger slide servo (220Hz -> 420Hz hum)
                if (t >= 0.04f && t < 0.18f)
                {
                    float tRel = t - 0.04f;
                    float servoFreq = 220.0f + 1600.0f * tRel;
                    pServo += XM_2PI * servoFreq / sampleRate;
                    float servoEnv = std::sin((tRel / 0.14f) * XM_PI);
                    sample += (std::sin(pServo) * 0.30f + std::sin(pServo * 2.0f) * 0.15f) * servoEnv;
                }

                // 0.14s - 0.32s: Heavy mechanical locking clank & metallic ring
                if (t >= 0.14f)
                {
                    float tRel = t - 0.14f;
                    pClank1 += XM_2PI * 520.0f / sampleRate; // Heavy body thud
                    pClank2 += XM_2PI * 1680.0f / sampleRate; // Metallic latch ring
                    float impactNoise = (tRel < 0.02f) ? noiseDist(rng) * 0.5f : 0.0f;

                    float env = std::exp(-22.0f * tRel);
                    sample += (std::sin(pClank1) * 0.50f + std::sin(pClank2) * 0.40f + impactNoise) * env;

                    // Electronic lock-in confirmation tone (2093Hz, C7)
                    if (tRel >= 0.05f)
                    {
                        float tBeep = tRel - 0.05f;
                        pBeep += XM_2PI * 2093.0f / sampleRate;
                        sample += std::sin(pBeep) * std::exp(-28.0f * tBeep) * 0.35f;
                    }
                }

                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.85f);
            }
            SaveWavFile("assets/audio/weapon_swap.wav", samples, sampleRate);
        }

        // 11. Heavy Bazooka: Earth-shattering explosive boom & low punch
        {
            float duration = 0.52f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float phase = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float freq = 85.0f * std::exp(-9.0f * t) + 28.0f;
                phase += XM_2PI * freq / sampleRate;

                float sub = std::sin(phase) * std::exp(-5.5f * t);
                float crack = noiseDist(rng) * std::exp(-28.0f * t);
                float lowRumble = noiseDist(rng) * (t < 0.25f ? 0.35f : 0.08f) * std::exp(-4.5f * t);

                float sample = (sub * 0.70f + crack * 0.40f + lowRumble * 0.35f);
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.95f);
            }
            SaveWavFile("assets/audio/shoot_bazooka.wav", samples, sampleRate);
        }

        // 12. Vertical Missile: Pneumatic pop + screaming rocket thruster acceleration
        {
            float duration = 0.38f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float pPop = 0.0f;
            float pTone = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;

                // Initial pop
                float popFreq = 340.0f * std::exp(-35.0f * t) + 60.0f;
                pPop += XM_2PI * popFreq / sampleRate;
                float pop = std::sin(pPop) * std::exp(-24.0f * t);

                // Rocket thrust hiss & scream
                float thrustFreq = 280.0f + 1400.0f * (t / duration);
                pTone += XM_2PI * thrustFreq / sampleRate;
                float hiss = noiseDist(rng) * (t > 0.03f ? 0.65f : 0.15f) * std::exp(-4.5f * t);
                float scream = std::sin(pTone) * 0.25f * (t > 0.04f ? 1.0f : 0.0f) * std::exp(-5.0f * t);

                float sample = pop * 0.60f + hiss * 0.45f + scream;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.88f);
            }
            SaveWavFile("assets/audio/shoot_missile.wav", samples, sampleRate);
        }

        // 13. Heavy Gatling: High-cycle mechanical rotor click + punchy kinetic crack
        {
            float duration = 0.11f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float pClick = 0.0f;
            float pBass = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                pClick += XM_2PI * 2400.0f / sampleRate;
                pBass  += XM_2PI * (210.0f * std::exp(-28.0f * t) + 55.0f) / sampleRate;

                float click = std::sin(pClick) * std::exp(-55.0f * t);
                float bass  = std::sin(pBass) * std::exp(-18.0f * t);
                float noise = noiseDist(rng) * std::exp(-32.0f * t) * 0.4f;

                float sample = bass * 0.65f + click * 0.35f + noise;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.85f);
            }
            SaveWavFile("assets/audio/shoot_gatling.wav", samples, sampleRate);
        }

        // 14. High-frequency Laser Blade: FM plasma ignition + space-cleaving swing
        {
            float duration = 0.40f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float pCar = 0.0f;
            float pMod = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float modFreq = 220.0f;
                float carFreq = 780.0f * std::exp(-7.0f * t) + 260.0f;
                float modIndex = 4.5f * std::exp(-6.0f * t);

                pMod += XM_2PI * modFreq / sampleRate;
                pCar += XM_2PI * carFreq / sampleRate;

                float fm = std::sin(pCar + std::sin(pMod) * modIndex);
                float whoosh = noiseDist(rng) * std::sin(XM_PI * (t / duration)) * 0.35f;

                float sample = (fm * 0.70f + whoosh) * std::exp(-5.5f * t);
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.88f);
            }
            SaveWavFile("assets/audio/blade_slash.wav", samples, sampleRate);
        }

        // 15. Spread Shotgun: Heavy simultaneous multi-pellet blast
        {
            float duration = 0.32f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float phase = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float freq = 180.0f * std::exp(-22.0f * t) + 40.0f;
                phase += XM_2PI * freq / sampleRate;

                float thump = std::sin(phase) * std::exp(-12.0f * t);
                float blast = noiseDist(rng) * (t < 0.06f ? 1.0f : 0.2f) * std::exp(-11.0f * t);

                float sample = thump * 0.55f + blast * 0.65f;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.90f);
            }
            SaveWavFile("assets/audio/shoot_shotgun.wav", samples, sampleRate);
        }

        // 16. Equip Change UI Chime
        {
            float duration = 0.22f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float pTone1 = 0.0f;
            float pTone2 = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                pTone1 += XM_2PI * 880.0f / sampleRate;
                pTone2 += XM_2PI * 1320.0f / sampleRate;

                float sample = (std::sin(pTone1) * 0.40f + std::sin(pTone2) * 0.35f) * std::exp(-14.0f * t);
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.75f);
            }
            SaveWavFile("assets/audio/equip_change.wav", samples, sampleRate);
        }

        // 17. Stagger Break: Heavy crushing crash + high-voltage electrical overload
        {
            float duration = 0.65f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float phaseLow = 0.0f;
            float phaseMetal = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                float freqLow = 95.0f * std::exp(-8.0f * t) + 28.0f;
                phaseLow += XM_2PI * freqLow / sampleRate;

                float freqMetal = 640.0f * std::exp(-4.0f * t) + 180.0f;
                phaseMetal += XM_2PI * freqMetal / sampleRate;

                float heavyThud = std::sin(phaseLow) * std::exp(-5.5f * t);
                float metalRing = (std::sin(phaseMetal) * 0.45f + std::sin(phaseMetal * 2.3f) * 0.25f) * std::exp(-7.0f * t);
                float electricalSparks = noiseDist(rng) * ((fmodf(t * 35.0f, 1.0f) < 0.35f) ? 0.65f : 0.10f) * std::exp(-4.5f * t);

                float sample = heavyThud * 0.55f + metalRing * 0.40f + electricalSparks * 0.45f;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.95f);
            }
            SaveWavFile("assets/audio/stagger_break.wav", samples, sampleRate);
        }

        // 18. Stagger Cockpit Alarm: Critical overload dual-beep
        {
            float duration = 0.32f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float pTone1 = 0.0f;
            float pTone2 = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                pTone1 += XM_2PI * 1480.0f / sampleRate;
                pTone2 += XM_2PI * 2220.0f / sampleRate;

                // Two quick pulses
                float pulse = (t < 0.12f || (t > 0.16f && t < 0.28f)) ? 1.0f : 0.0f;
                float sample = (std::sin(pTone1) * 0.60f + std::sin(pTone2) * 0.30f) * pulse;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.80f);
            }
            SaveWavFile("assets/audio/stagger_alarm.wav", samples, sampleRate);
        }

        // 19. System Restored: Upward sci-fi recovery boot chime
        {
            float duration = 0.48f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float phase = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                // Upward ascending arpeggiated glissando: 523Hz (C5) -> 659Hz (E5) -> 784Hz (G5) -> 1046Hz (C6)
                float freq = (t < 0.12f) ? 523.25f : ((t < 0.24f) ? 659.25f : ((t < 0.36f) ? 783.99f : 1046.50f));
                phase += XM_2PI * freq / sampleRate;

                float sample = (std::sin(phase) * 0.70f + std::sin(phase * 2.0f) * 0.20f) * std::exp(-3.5f * fmodf(t, 0.12f));
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.82f);
            }
            SaveWavFile("assets/audio/system_restored.wav", samples, sampleRate);
        }

        // 20. Boost Kick Thrust: High-energy rocket engine blast & sudden acceleration
        {
            float duration = 0.42f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float phaseLow = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                // Descending deep rocket drone + noisy afterburner rush
                float freq = 120.0f * std::exp(-5.0f * t) + 38.0f;
                phaseLow += XM_2PI * freq / sampleRate;

                float lowRumble = std::sin(phaseLow) * 0.70f;
                float thrustNoise = noiseDist(rng) * (0.45f + 0.35f * std::exp(-4.0f * t));
                float env = std::min(1.0f, t * 25.0f) * std::exp(-2.8f * t);

                float sample = (lowRumble * 0.55f + thrustNoise * 0.45f) * env;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.90f);
            }
            SaveWavFile("assets/audio/boost_kick_thrust.wav", samples, sampleRate);
        }

        // 21. Boost Kick Hit: Megaton kinetic collision impact + metallic crumple & electrical crush
        {
            float duration = 0.55f;
            int totalSamples = static_cast<int>(sampleRate * duration);
            std::vector<int16_t> samples(totalSamples);
            float phaseSub = 0.0f;
            float phaseMetal = 0.0f;

            for (int i = 0; i < totalSamples; ++i)
            {
                float t = static_cast<float>(i) / sampleRate;
                // Deep sub-bass compression shockwave (60Hz -> 20Hz)
                float freqSub = 60.0f * std::exp(-7.0f * t) + 20.0f;
                phaseSub += XM_2PI * freqSub / sampleRate;

                // Metallic crunch & buckle clang (420Hz -> 140Hz)
                float freqMetal = 420.0f * std::exp(-5.0f * t) + 140.0f;
                phaseMetal += XM_2PI * freqMetal / sampleRate;

                float subHit = std::sin(phaseSub) * std::exp(-4.5f * t);
                float metalCrunch = (std::sin(phaseMetal) * 0.5f + std::sin(phaseMetal * 2.1f) * 0.3f) * std::exp(-6.5f * t);
                float impactNoise = noiseDist(rng) * std::exp(-8.0f * t) * 0.85f;
                float sparks = noiseDist(rng) * ((fmodf(t * 40.0f, 1.0f) < 0.3f) ? 0.40f : 0.05f) * std::exp(-3.5f * t);

                float sample = subHit * 0.60f + metalCrunch * 0.45f + impactNoise * 0.40f + sparks * 0.30f;
                samples[i] = static_cast<int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f * 0.98f);
            }
            SaveWavFile("assets/audio/boost_kick_hit.wav", samples, sampleRate);
        }
    }
}
