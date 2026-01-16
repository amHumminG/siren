#include "siren/Voice.h"
#include "internal/log.h"
#include "siren/AudioBus.h"
#include <algorithm>
#include <string>
#include <array>
#include <cmath>
#include "siren/SirenMath.h"

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

		bool isLooping = m_isLooping.load();
		float gainL = m_gainL.load();
		float gainR = m_gainR.load();

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

	void Voice::update(float deltaTime, const ListenerData& listener) {
		float vol = 1.0f;
		float pan = 0.0f;
		if (m_mode == VoiceMode::Spatial) {
			// Distance based volume
			float distance = (m_position - listener.position).length();
			distance = std::clamp(distance, m_minDistance, m_maxDistance);
			float fraction = (distance - m_minDistance) / (m_maxDistance - m_minDistance);
			vol = 1.0f - fraction;

			// Listener based panning
			Vector3 listenerToEmitter = normalize(m_position - listener.position);
			Vector3 right = listener.right;

			pan = right * listenerToEmitter;
		}
		else {
			pan = m_pan.load();
		}

		float panNormalized = (pan + 1.0f) * 0.5f;
		float angle = panNormalized * static_cast<float>(M_PI_2); // Angle between 0 and PI/2 radians

		// Apply pan and volume
		float gainL = std::cos(angle) * vol;
		float gainR = std::sin(angle) * vol;
		m_gainL.store(gainL);
		m_gainR.store(gainR);

		// Velocity
		if (m_velocitySetThisFrame) {
			m_velocitySetThisFrame = false;
		}
		else if (m_firstUpdate) {
			m_velocity = { 0.0f, 0.0f, 0.0f };
			m_firstUpdate = false;
		}
		else {
			// Approximate voice velocity
			Vector3 distance = m_position - m_previousPosition;
			m_velocity = distance / deltaTime;
		}

		m_previousPosition = m_position;

		// TODO: Use velocity for doppler effect
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

	void Voice::setPosition(const Vector3& pos) {
		m_position = pos;
		m_mode = VoiceMode::Spatial;
	}

	void Voice::setVelocity(const Vector3& vel) {
		m_velocity = vel;
		m_velocitySetThisFrame = true;
	}

	Vector3 Voice::getVelocity() const {
		return m_velocity;
	}

	void Voice::setGlobal() {
		m_mode = VoiceMode::Global;
		m_pan.store(0.0f); // Reset pan
	}

	void Voice::setDistance(float minDistance, float maxDistance) {
		if (minDistance < 0.1f) {
			minDistance = 0.1f;
		}
		if (maxDistance < 0.1f || minDistance > maxDistance) {
			maxDistance = minDistance + 0.1f;
		}

		m_minDistance = minDistance;
		m_maxDistance = maxDistance;
	}

	float Voice::getMinDistance() {
		return m_minDistance;
	}

	float Voice::getMaxDistance() {
		return m_maxDistance;
	}
}
