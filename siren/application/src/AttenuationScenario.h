#pragma once
#include "Scenario.h"
#include "siren/AudioContext.h"

class AttenuationScenario : public Scenario {
public:
    // UI State Variables
    float m_emitterZ = 0.0f;          // Position of sound on Z axis
    float m_minDist = 5.0f;           // "Full Volume" Radius
    float m_maxDist = 200.0f;         // "Silence" Radius
    float m_volume = 1.0f;
    bool m_doppler = true;
    int m_selectedModel = 1;          // 0:None, 1:Linear, 2:Inverse, 3:Exp
    float m_rolloff = 1.0f;

    bool m_autoMove = false;          // Toggle for automatic movement
    float m_moveSpeed = 10.0f;        // Meters per second
    float m_moveDir = 1.0f;           // 1.0 = Away, -1.0 = Towards
    float m_maxZ = 300.0f;

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

        if (m_autoMove) {
            // Move based on speed and direction
            m_emitterZ += m_moveSpeed * m_moveDir * deltaTime;

            // Bounce check (Ping Pong between 0 and 100)
            if (m_emitterZ >= m_maxZ) {
                m_emitterZ = m_maxZ;
                m_moveDir = -1.0f; // Reverse direction
            }
            else if (m_emitterZ <= 0.0f) {
                m_emitterZ = 0.0f;
                m_moveDir = 1.0f;  // Reverse direction
            }
        }

        m_voice->setVolume(m_volume);

        // Apply Position (Sliding the emitter away from listener)
        m_voice->setPosition(siren::Vector3(0, 0, m_emitterZ));

        // Apply Physics Settings Dynamically
        m_voice->setDistance(m_minDist, m_maxDist);

        m_voice->setDopplerEffect(m_doppler);

        // Apply Attenuation Model
        auto model = static_cast<siren::Voice::AttenuationModel>(m_selectedModel);
        m_voice->setAttenuationModel(model);
        m_voice->setRolloff(m_rolloff);
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
        ImGui::SeparatorText("Emitter Movement");

        // Toggle Auto Move
        ImGui::Checkbox("Auto Ping-Pong (0m <-> 100m)", &m_autoMove);
        ImGui::Text("Maximum Travel Distance");
        ImGui::SliderFloat("###Maximum Travel Distance", &m_maxZ, 100.0f, 1000.0f);

        if (m_autoMove) {
            // Show Speed slider when auto is active
            ImGui::SliderFloat("Move Speed (m/s)", &m_moveSpeed, 1.0f, 100.0f);

            // Show the Z Slider as "Read Only" (Greyed out) so you can watch the value change
            ImGui::BeginDisabled();
            ImGui::Text("Emitter Distance (Z)");
            ImGui::SliderFloat("###Emitter Distance (Z)", &m_emitterZ, 0.0f, m_maxZ);
            ImGui::EndDisabled();
        }
        else {
            // Normal manual control
            ImGui::Text("Emitter Distance (Z)");
            ImGui::SliderFloat("###Emitter Distance (Z)", &m_emitterZ, 0.0f, m_maxZ);
        }

        ImGui::SeparatorText("Emitter Settings");
        ImGui::SliderFloat("Volume", &m_volume, 0.0f, 1.0f);
        ImGui::Checkbox("Doppler Effect", &m_doppler);

        ImGui::SliderFloat("Min Distance", &m_minDist, 0.1f, 100.0f);
        ImGui::SliderFloat("Max Distance", &m_maxDist, m_minDist + 1.0f, 1000.0f);
        ImGui::SliderFloat("Rolloff", &m_rolloff, 0.0f, 10.0f);

        // -- Model Switcher --
        if (ImGui::Button("Linear"))      m_selectedModel = 1;
        if (ImGui::Button("Inverse"))     m_selectedModel = 2;
        if (ImGui::Button("Exponential")) m_selectedModel = 3;
    }
};