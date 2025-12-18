#include "siren/Voice.h"

namespace siren {

	void Voice::attachDecoder(std::unique_ptr<Decoder> decoder) {
		std::lock_guard<std::mutex> lock(m_mutex); // Lock

		m_state = VoiceState::Inactive;
		m_isLooping = false;
		m_decoder = std::move(decoder);
	}

	void Voice::process(std::span<float> dst) {
		// Has to be somewhat thread-safe

		std::unique_lock<std::mutex> lock(m_mutex, std::try_to_lock);
		if (!lock.owns_lock()) {
			std::fill(dst.begin(), dst.end(), 0.0f);
			return;
		}

		// TODO: Implement support for Mono. For mono to stereo up-mixing,
		// we will need an intermediate buffer to hold the raw mono before expanding it to stereo

		// Getting all variables at once (for thread safety)
		float volume = m_volume;
		float pan = m_pan;
		bool isLooping = m_isLooping;
		size_t channelCount = m_decoder->getChannelCount();

		if (m_state != VoiceState::Playing || !m_decoder) {
			std::fill(dst.begin(), dst.end(), 0.0f);
			return;
		}

		size_t framesRequested = dst.size() / channelCount;
		size_t framesRead = 0;

		while (framesRead < framesRequested) {

			size_t framesRemaining = framesRequested - framesRead;
			size_t samplesRead = framesRead * channelCount;
			size_t samplesRemaining = framesRemaining * channelCount;

			std::span<float> buffer = dst.subspan(samplesRead, samplesRemaining);
			framesRead += m_decoder->decode(buffer);

			if (framesRead < framesRequested) {
				// Hit EOF
				if (m_isLooping) {
					// Continue filling the buffer from the start
					m_decoder->seek(0);
					continue;
				}
				else {
					// Fill rest of the buffer with silence
					samplesRead = framesRead * channelCount;
					std::fill(dst.begin() + samplesRead, dst.end(), 0.0f);
					m_state = VoiceState::Inactive;
					break;
				}
			}
		}

		for (size_t i = 0; i < dst.size(); i++) {
			// Apply volume
			dst[i] *= volume;

			if (channelCount == 2) {
				// Apply pan
				if (i % 2 == 0) { // Left
					dst[i] *= (1.0 - pan);
				}
				else { // Right
					dst[i] *= (1.0 + pan);
				}
			}
		}
	}



}


