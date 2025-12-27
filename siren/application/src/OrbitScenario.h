#pragma once
#include "Scenario.h"
#include "siren/AudioContext.h"

class OrbitScenario : public Scenario {
private:
	std::shared_ptr<siren::Voice> m_voice = nullptr;

	// Logic
	float m_angle = 0.0f;
	float m_radius = 10.0f;
	float m_speed = 0.5f;
	float m_height = 2.0f;
	bool m_isOrbiting = true;

	// Sound properties
	float m_minDist = 5.0f;
	float m_maxDist = 25.0f;
	Vector3 m_pos = { 0.0f, 0.0f, 0.0f };

public:
	std::string getName() override {
		return "Orbit";
	}

	void onStart(siren::AudioContext& context) override {
		siren::Sound sound = siren::Sound::Stream("assets/audio/music/white_ferrari.wav");
		m_voice = context.play(sound, "Music");

		if (m_voice) {
			m_voice->setLooping(true);
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
		if (!m_voice) return;

		// Orbit calculation
		if (m_isOrbiting) {
			m_angle += m_speed * deltaTime;
			m_pos.x = std::sin(m_angle) * m_radius;
			m_pos.y = m_height;
			m_pos.z = std::cos(m_angle) * m_radius;
		}

		m_voice->setPosition({ m_pos.x, m_pos.y, m_pos.z });
		m_voice->setDistance(m_minDist, m_maxDist);
	}

	void draw3D() override {
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
		ImGui::Separator();

		ImGui::Text("Motion");
		ImGui::Checkbox("Orbit Active", &m_isOrbiting);
		if (m_isOrbiting) {
			ImGui::SliderFloat("Speed", &m_speed, 0.0f, 10.0f);
			ImGui::SliderFloat("Radius", &m_radius, 0.0f, 50.0f);
		}
		else {
			ImGui::DragFloat3("Position", &m_pos.x, 0.1f);
		}
		ImGui::SliderFloat("Height", &m_height, -10.0f, 20.0f);

		ImGui::Separator();
		ImGui::Text("Attenuation");
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Min Distance (Green)");
		ImGui::SliderFloat("MinDist", &m_minDist, 0.1f, 20.0f);
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Max Distance (Gray)");
		ImGui::SliderFloat("MaxDist", &m_maxDist, m_minDist, 100.0f);
	}
};
