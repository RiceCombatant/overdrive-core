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
        loadSoundPool("assets/audio/quick_boost.wav", m_qbSounds,         true);
        loadSoundPool("assets/audio/explosion.wav",   m_explosionSounds,  true);
        loadSoundPool("assets/audio/lock_on.wav",     m_lockOnSounds,     false); // 2D cockpit sound

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
        cleanupPool(m_qbSounds);
        cleanupPool(m_explosionSounds);
        cleanupPool(m_lockOnSounds);

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
        bool qb     = std::filesystem::exists("assets/audio/quick_boost.wav");
        bool explo  = std::filesystem::exists("assets/audio/explosion.wav");
        bool lock   = std::filesystem::exists("assets/audio/lock_on.wav");
        bool bLoop  = std::filesystem::exists("assets/audio/boost_loop.wav");

        if (!shootR || !shootL || !qb || !explo || !lock || !bLoop)
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
    }
}
