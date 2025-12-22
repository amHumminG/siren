#include "siren/Voice.h"
#include "internal/log.h"
#include "siren/AudioBus.h"
#include <algorithm>
#include <string>
#include <array>
#include <cmath>

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

namespace siren {

	void Voice::attachDecoder(std::unique_ptr<Decoder> decoder) {
		m_state.store(VoiceState::Inactive);
		m_isLooping.store(false);
		m_sampleRate = decoder->getSampleRate();
		m_decoder = std::move(decoder);
	}

	bool Voice::mix() {
		VoiceState state = m_state.load();
		if (state == VoiceState::Inactive || !m_decoder) {
			return false; // Dead
		}

		if (state == VoiceState::Paused) {
			return true; // Still alive
		}

		// Handle seek requests
		int64_t seekRequest = m_seekFrame.exchange(-1);
		if (seekRequest >= 0 && m_decoder) {
			m_decoder->seek(static_cast<size_t>(seekRequest));
		}

		if (!m_bus) {
			return false;
		}
		std::span<float> dst = m_bus->m_buffer;

		constexpr size_t BUFFER_FRAMES = 256;
		std::array<float, BUFFER_FRAMES * 2> intermediateBuffer;

		size_t framesRequested = dst.size() / 2;
		size_t framesRead = 0;
		size_t decoderChannelCount = m_decoder->getChannelCount();

		float pan = m_pan.load();
		bool isLooping = m_isLooping.load();

		float panNormalized = (pan + 1.0f) * 0.5f;
		float angle = panNormalized * static_cast<float>(M_PI_2); // Angle between 0 and PI/2 radians

		float gainL = std::cos(angle);
		float gainR = std::sin(angle);

		while (framesRead < framesRequested) {

			size_t framesToDecode = std::min(framesRequested - framesRead, BUFFER_FRAMES);
			std::span<float> bufferView(intermediateBuffer.data(), framesToDecode * decoderChannelCount);

			size_t framesDecoded = m_decoder->decode(bufferView);
			size_t framesThisIteration = framesDecoded;

			if (framesDecoded < framesToDecode) {
				// Hit EOF
				if (isLooping) {
					// Start from beginning
					if (m_decoder->seek(0) != ResultCode::Success) {
						return false;
					}
				}
				else {
					m_state.store(VoiceState::Inactive); // Audio clip over
				}
			}

			// Mix and add to destination buffer
			for (size_t i = 0; i < framesThisIteration; i++) {
				float sampleL;
				float sampleR;

				if (decoderChannelCount == 1) {
					// Mono
					sampleL = intermediateBuffer[i];
					sampleR = intermediateBuffer[i];
				}
				else {
					// Stereo
					sampleL = intermediateBuffer[i * 2];
					sampleR = intermediateBuffer[i * 2 + 1];
				}

				sampleL *= gainL;
				sampleR *= gainR;

				size_t dstIndex = (framesRead + i) * 2;
				dst[dstIndex]		+= sampleL;
				dst[dstIndex + 1]	+= sampleR;
			}

			framesRead += framesThisIteration;
			if (m_state.load() != VoiceState::Playing) {
				break;
			}
		}
		return m_state.load() != VoiceState::Inactive;
	}

	void Voice::play() {
		if (m_decoder) {
			if (m_state.load() == VoiceState::Inactive) {
				ResultCode result = m_decoder->seek(0);
				if (result != ResultCode::Success) {
					std::string error = std::to_string((int)result);
					SIREN_LOG_ERROR("Voice::play() Failed to seek. ERROR: " << (int)result);
					return;
				}
			}
			m_state.store(VoiceState::Playing);
		}
	}

	void Voice::pause() {
		m_state.store(VoiceState::Paused);
	}

	void Voice::stop() {
		m_state.store(VoiceState::Inactive);
	}

	void Voice::setBus(AudioBus* bus) {
		m_bus = bus;
	}

	void Voice::setPan(float value) {
		m_pan.store(std::clamp(value, -1.0f, 1.0f));
	}

	void Voice::setLooping(bool value) {
		m_isLooping.store(value);
	}

	void Voice::setTag(const std::string& tag) {
		m_tag = tag;
	}

	const std::string& Voice::getTag() {
		return m_tag;
	}

	void Voice::seek(float timePoint) {
		// TODO: Check if timePoint is out of bounds
		// This will require that we store totalFrames in voice as a memeber variable
		if (m_state.load() == VoiceState::Inactive) {
			return;
		}
		int64_t frame = static_cast<int64_t>(timePoint * m_sampleRate);
		m_seekFrame.store(frame);
	}

	float Voice::getPan() const {
		return m_pan.load();
	}

	bool Voice::isLooping() const {
		return m_isLooping.load();
	}

	bool Voice::isPlaying() const {
		return m_state.load() == VoiceState::Playing;
	}
}
