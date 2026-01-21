#pragma once
#include "Scenario.h"
#include "siren/AudioContext.h"

class PitchScenario : public Scenario {
private:
	std::shared_ptr<siren::Voice> m_voice = nullptr;
	Vector3 m_pos = { 0.0f, 5.0f, 10.0f };
	float m_speed = 25.0f;
	float m_range = 100.0f;
	float m_direction = 1.0f;
	bool m_isMoving = true;

	float m_minDist = 5.0f;
	float m_maxDist = 100.0f;

	float m_pitch = 1.0f;
	bool m_useDoppler = true;
	float m_dopplerFactor = 1.0f;

public:
	std::string getName() override {
		return "Pitch";
	}

	void onStart(siren::AudioContext& context) override {
		//siren::Sound sound = siren::Sound::Internal("assets/audio/music/blood_run_warm.wav");
		siren::Sound sound = siren::Sound::Internal("assets/audio/sfx/400hz.wav");
		m_voice = context.play(sound, "SFX");

		if (m_voice) {
			m_voice->setLooping(true);
			m_voice->setVolume(0.5f);
			m_voice->setDistance(m_minDist, m_maxDist);
		}
	}

	void onStop(siren::AudioContext& context) override {
		if (m_voice) {
			m_voice->stop();
			m_voice = nullptr;
		}
	}

	void update(float deltaTime, siren::AudioContext& context) override {
		if (m_isMoving) {
			m_pos.x += (m_speed * m_direction) * deltaTime;

			if (m_pos.x > m_range) {
				m_pos.x = m_range;
				m_direction = -m_direction;
			}
			if (m_pos.x < -m_range) {
				m_pos.x = -m_range;
				m_direction = -m_direction;
			}
			m_voice->setPosition({ m_pos.x, m_pos.y, m_pos.y });
		}
		m_voice->setDistance(m_minDist, m_maxDist);

		m_voice->setPitch(m_pitch);
		m_voice->setDopplerEffect(m_useDoppler);
		if (m_useDoppler) m_voice->setDopplerFactor(m_dopplerFactor);
	}

	void draw3D() override{
		DrawSphere(m_pos, 0.5f, RED);
		DrawPoint3D(m_pos, RED);

		DrawSphereWires(m_pos, m_minDist, 16, 16, DARKGREEN);
		DrawSphereWires(m_pos, m_maxDist, 16, 16, DARKGRAY);

		// Ground tether
		DrawLine3D(m_pos, { m_pos.x, 0.0f, m_pos.z }, YELLOW);
	}

	void drawUI() override {
		if (!m_voice) {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "ERROR: Voice not playing");
			return;
		}

		ImGui::Text("Status:");
		ImGui::SameLine();
		if (m_voice->isPlaying()) {
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Playing");
		}
		else {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Stopped");
		}

		ImGui::SeparatorText("Motion");
		ImGui::Checkbox("Movement Active", &m_isMoving);
		if (m_isMoving) {
			ImGui::SliderFloat("Speed", &m_speed, 0.0f, 50.0f);
			ImGui::SliderFloat("Range", &m_range, 5.0f, 200.0f);
		}
		else {
			ImGui::DragFloat3("Position", &m_pos.x, 0.1f);
		}

		ImGui::SeparatorText("Pitch Modifiers");
		ImGui::Text("Manual Pitch");
		if (ImGui::Button("Reset ##1")) m_pitch = 1.0f;
		ImGui::SameLine();
		ImGui::SliderFloat("##Pitch", &m_pitch, 0.1f, 4.0f);
		ImGui::Checkbox("Doppler Effect", &m_useDoppler);
		if (m_useDoppler) {
			ImGui::Text("Doppler Factor");
			if (ImGui::Button("Reset ##2")) m_dopplerFactor = 1.0f;
			ImGui::SameLine();
			ImGui::SliderFloat("##Doppler Factor", &m_dopplerFactor, 0.1f, 4.0f);
		}

		siren::Vector3 voiceVelocity = m_voice.get()->getVelocity();
		ImGui::BeginDisabled();
		float vel[3] = { voiceVelocity.x, voiceVelocity.y, voiceVelocity.z };
		ImGui::InputFloat3("Velocity", vel, "%.2f", ImGuiInputTextFlags_ReadOnly);
		ImGui::EndDisabled();

		// Calculate Speed (Magnitude)
		float speed = std::sqrt(
			voiceVelocity.x * voiceVelocity.x +
			voiceVelocity.y * voiceVelocity.y +
			voiceVelocity.z * voiceVelocity.z
		);

		ImGui::Text("Speed: ");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "%.2f m/s", speed);

		ImGui::SeparatorText("Attenuation");
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Min Distance (Green)");
		ImGui::SliderFloat("##MinDist", &m_minDist, 0.1f, 20.0f);
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Max Distance (Gray)");
		ImGui::SliderFloat("##MaxDist", &m_maxDist, m_minDist, 200.0f);
	}
};