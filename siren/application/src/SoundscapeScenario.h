#pragma once
#include "Scenario.h"
#include "siren/AudioContext.h"

#include <vector>
#include <array>
#include <random>
#include <cmath>

struct SoundEffect {
	std::shared_ptr<siren::Voice> voice;
	Vector3 position;
	Color color;
};

class SoundscapeScenario : public Scenario {
private:
	std::shared_ptr<siren::Voice> m_soundtrackVoice = nullptr;
	bool m_soundtrackPlaying = false;

	std::vector<std::unique_ptr<siren::Sound>> m_sfxSounds;
	std::vector<SoundEffect> m_sfx;

	float m_trySpawnInterval = 1.0f;
	float m_spawnChance = 0.25f;

	float m_spawnRange = 50.0f; // random range (-m_range -> m_range)
	std::mt19937 m_gen;

public:
	std::string getName() override {
		return "Soundscape";
	}

	void onStart(siren::AudioContext& context) override {
		// Random initialization
		m_gen = std::mt19937(std::random_device{}());

		// Play soundtrack
		siren::Sound sound = siren::Sound::Stream("assets/audio/music/lurks_below_theme.wav");
		m_soundtrackVoice = context.play(sound, "Music");
		if (m_soundtrackVoice) {
			m_soundtrackVoice->setLooping(true);
			m_soundtrackPlaying = true;
		}

		// Create sounds for sfx
		std::array<std::string, 5> sfxPaths = {
			"assets/audio/sfx/air_horn.wav",
			"assets/audio/sfx/cinematic_boom.wav",
			"assets/audio/sfx/fah.wav",
			"assets/audio/sfx/metal_pipe.wav",
			"assets/audio/sfx/plankton_augh.wav"
		};
		for (auto& path : sfxPaths) {
			m_sfxSounds.push_back(std::make_unique<siren::Sound>(siren::Sound::Stream(path)));
		}
	}

	void onStop(siren::AudioContext& context) override {
		if (m_soundtrackVoice) {
			m_soundtrackVoice->stop();
		}

		for (auto& effect : m_sfx) {
			if (effect.voice) {
				effect.voice->stop();
			}
		}
		m_sfx.clear();
	}

	void update(float deltaTime, siren::AudioContext& context) override {
		if (m_soundtrackPlaying) {
			if (!m_soundtrackVoice->isPlaying()) {
				m_soundtrackVoice->play();
			}
		}
		else {
			if (m_soundtrackVoice->isPlaying()) {
				m_soundtrackVoice->pause();
			}
		}

		// Randomly generate and play new sfx at random locations within set units of origo
		static float spawnTimer = 0.0f;
		spawnTimer += deltaTime;
		if (spawnTimer >= m_trySpawnInterval) {
			spawnTimer -= m_trySpawnInterval;

			// Try to spawn sfx at random point
			std::uniform_real_distribution spawnDistribution(-m_spawnRange, m_spawnRange);
			if (abs(spawnDistribution(m_gen)) <= (m_spawnChance * m_spawnRange)) {
				size_t numSounds = m_sfxSounds.size() - 1;
				std::uniform_int_distribution indexDistribution(0, static_cast<int>(numSounds));
				size_t index = indexDistribution(m_gen);
				
				SoundEffect effect;
				effect.voice = context.play(*m_sfxSounds.at(index).get(), "SFX");
				effect.position = Vector3(
					spawnDistribution(m_gen),
					spawnDistribution(m_gen),
					spawnDistribution(m_gen)
				);
				std::uniform_int_distribution colorDistrib(0, 255);
				effect.color = Color{ 
					static_cast<unsigned char>(colorDistrib(m_gen)), 
					static_cast<unsigned char>(colorDistrib(m_gen)),
					static_cast<unsigned char>(colorDistrib(m_gen)),
					255
				};

				effect.voice->setPosition({ 
					effect.position.x,
					effect.position.y,
					effect.position.z
					});

				m_sfx.push_back(effect);
			}
		}

		// Remove the sfx that stopped playing
		auto it = m_sfx.begin();
		while (it != m_sfx.end()) {
			if (!it->voice->isPlaying()) {
				it = m_sfx.erase(it);
			}
			else {
				it++;
			}
		}

	}

	void draw3D() override {
		for (auto& effect : m_sfx) {
			DrawSphereWires(effect.position, effect.voice->getMinDistance(), 16, 16, effect.color);

			// Ground tether
			DrawLine3D(effect.position, { effect.position.x, 0.0f, effect.position.z }, YELLOW);
		}
	}

	void drawUI() override {
		// Show all sfx playing and their location

		ImGui::Text("Status:");
		ImGui::SameLine();
		if (m_soundtrackVoice->isPlaying()) {
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Playing");
		}
		else {
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Paused/Stopped");
		}

		ImGui::Checkbox("Soundtrack Active", &m_soundtrackPlaying);

		ImGui::SeparatorText("Sound Effects");

		ImGui::Text("Effects: %zu", m_sfx.size());
		ImGui::Text("Spawn Range (From Origo)");
		ImGui::SliderFloat("##SpawnRange", &m_spawnRange, 0.10f, 50.0f);
		ImGui::Text("Spawn Try Interval (Seconds)");
		ImGui::SliderFloat("##SpawnTry", &m_trySpawnInterval, 0.1f, 5.0f);
		ImGui::Text("Spawn Chance (Per Try)");
		ImGui::SliderFloat("##SpawnChance", &m_spawnChance, 0.1f, 1.0f);
	}
};