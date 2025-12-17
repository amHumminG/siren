#pragma once
#include "siren/DecoderFactory.h"

enum VoiceState {
	Inactive,
	Playing,
	Paused
};

namespace siren {

	class Voice {
	private:
		std::unique_ptr<Decoder> decoder;

		VoiceState m_state = VoiceState::Inactive;
		
		float m_volume = 1.0f;
		float m_pan = 0.0f;
		bool m_isLooping = false;

	public:
		Voice() = default;
		~Voice() = default;

		[[nodiscard]] ResultCode attachDecoder(std::unique_ptr<Decoder> decoder);


		[[nodiscard]] size_t process(std::span<std::byte> dst);

		void play();
		void pause();
		void stop(); // Should this have a return value?

		void setVolume(const float& value);
		void setPan(const float& value);
		void setLooping(const bool& value);

		[[nodiscard]] float getVolume();
		[[nodiscard]] float getPan();
		[[nodiscard]] bool isLooping();

		[[nodiscard]] bool isPlaying();
	};
}