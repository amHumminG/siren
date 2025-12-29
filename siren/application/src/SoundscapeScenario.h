#pragma once
#include "Scenario.h"
#include "siren/AudioContext.h"

#include <vector>
#include <array>

struct SoundEffect {
	siren::Voice* voice;
	Vector3 position;
};

class SoundscapeScenario : public Scenario {
private:
	std::shared_ptr<siren::Voice> m_soundtrackVoice = nullptr;

	std::vector<std::unique_ptr<siren::Sound>> m_sfxSounds;
	std::vector<SoundEffect> m_sfx;

public:
	std::string getName() override {
		return "Soundscape";
	}

	void onStart(siren::AudioContext& context) override {
		// Play soundtrack
		siren::Sound sound = siren::Sound::Stream("assets/audio/music/lurks_below_theme.wav");
		m_soundtrackVoice = context.play(sound, "Music");
		if (m_soundtrackVoice) {
			m_soundtrackVoice->setLooping(true);
		}

		// Create sounds for sfx
		std::array<std::string, 5> sfxPaths = {
			"assets/audio/sfx/air_horn.wav.wav",
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
		// Randomly generate and play new sfx at random locations within 50 units of origo
		// Remove the sfx that stopped playing
	}

	void draw3D() override {
		// Draw the sfx in m_sfx
	}

	void drawUI() override {
		// Show all sfx playing and their location
	}

};