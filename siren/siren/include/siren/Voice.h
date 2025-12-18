#pragma once
#include "siren/Decoder.h"
#include <atomic>
#include <mutex>

enum VoiceState {
	Inactive,
	Playing,
	Paused
};

namespace siren {

	class Voice {
	private:
		std::mutex m_mutex;
		std::unique_ptr<Decoder> m_decoder;
		
		std::atomic<VoiceState> m_state{ VoiceState::Inactive };
		std::atomic<float> m_volume{ 1.0f };
		std::atomic<float> m_pan{ 0.0f };
		std::atomic<float> m_isLooping{ false };

	public:
		Voice() = default;
		~Voice() = default;

		// TODO: Add documentation

		void attachDecoder(std::unique_ptr<Decoder> decoder);

		void process(std::span<float> dst);

		void play();
		void pause();
		void stop();

		void setVolume(float value);
		void setPan(float value);
		void setLooping(bool value);

		[[nodiscard]] float getVolume();
		[[nodiscard]] float getPan();
		[[nodiscard]] bool isLooping();
		[[nodiscard]] bool isPlaying();
	};
}