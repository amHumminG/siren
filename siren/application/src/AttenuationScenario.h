#pragma once
#include "Scenario.h"
#include "siren/AudioContext.h"

class AttenuationScenario : public Scenario {
public:
    // UI State Variables
    float m_emitterZ = 0.0f;          // Position of sound on Z axis
    float m_minDist = 5.0f;           // "Full Volume" Radius
    float m_maxDist = 50.0f;          // "Silence" Radius
    float m_volume = 1.0f;
    int m_selectedModel = 1;          // 0:None, 1:Linear, 2:Inverse, 3:Exp

    std::shared_ptr<siren::Voice> m_voice;

    std::string getName() override {
        return "Attenuation";
    }

    void onStart(siren::AudioContext& context) override {
        // Load a constant loop (Engine hum or White Noise is best for this)
        siren::Sound sound = siren::Sound::Stream("assets/audio/music/blood_run_warm.wav");

        std::shared_ptr<siren::AudioBus> sfxBus = context.getBus("SFX");
        m_voice = context.createVoice(sound, sfxBus);
        if (m_voice) {
            m_voice->setLooping(true);
            m_voice->setVolume(1.0f);
            m_voice->play();
        }
    }

    void onStop(siren::AudioContext& context) override {
        if (m_voice) {
            m_voice->stop();
            m_voice = nullptr;
        }
    }

    // 2. The Logic Loop (Call this every frame)
    void update(float deltaTime, siren::AudioContext& context) override {
        if (!m_voice) return;

        m_voice->setVolume(m_volume);

        // Apply Position (Sliding the emitter away from listener)
        m_voice->setPosition(siren::Vector3(0, 0, m_emitterZ));

        // Apply Physics Settings Dynamically
        m_voice->setDistance(m_minDist, m_maxDist);

        // Apply Attenuation Model
        auto model = static_cast<siren::Voice::AttenuationModel>(m_selectedModel);
        m_voice->setAttenuationModel(model);
    }

    void draw3D() override {
        DrawSphere(Vector3(0, 0, m_emitterZ), 0.5f, RED);
        DrawPoint3D(Vector3(0, 0, m_emitterZ), RED);

        DrawSphereWires(Vector3(0, 0, m_emitterZ), m_minDist, 16, 16, DARKGREEN);
        DrawSphereWires(Vector3(0, 0, m_emitterZ), m_maxDist, 16, 16, DARKGRAY);

        // Ground tether
        DrawLine3D(Vector3(0, 0, m_emitterZ), { 0.0f, 0.0f, m_emitterZ }, YELLOW);
    }

    void drawUI() override {
        // -- Position Control --
        ImGui::SliderFloat("Emitter Distance (Z)", &m_emitterZ, 0.0f, 100.0f);
        ImGui::SliderFloat("Volume", &m_volume, 0.0f, 1.0f);

        // -- Physics Controls --
        ImGui::SliderFloat("Min Distance (Full Vol)", &m_minDist, 0.1f, 20.0f);
        ImGui::SliderFloat("Max Distance (Silence)", &m_maxDist, m_minDist + 1.0f, 200.0f);

        // -- Model Switcher --
        if (ImGui::Button("Linear"))      m_selectedModel = 1;
        if (ImGui::Button("Inverse"))     m_selectedModel = 2;
        if (ImGui::Button("Exponential")) m_selectedModel = 3;

        ImGui::Text(("Current Volume: " + std::to_string(m_voice->getVolume())).c_str());
    }
};