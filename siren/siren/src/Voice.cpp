#include "siren/Voice.h"
#include "internal/log.h"
#include "siren/AudioBus.h"
#include <algorithm>
#include <string>
#include <array>
#include <cmath>
#include <thread>

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

constexpr float SPEED_OF_SOUND = 343.0f; // m/s

namespace siren {

	void Voice::play() {
		if (m_decoder) {
			if (m_state.load(std::memory_order_relaxed) == VoiceState::Destroyed) {
				SIREN_LOG_ERROR("Voice::play() Unable to play destroyed voice");
				return;
			}

			if (!isPlaying()) {
				m_isReusable.store(true, std::memory_order_relaxed);
				m_snapGainRequested.store(true, std::memory_order_relaxed);
				m_state.store(VoiceState::Playing, std::memory_order_release);
			}
		}
	}

	void Voice::playOneShot() {
		if (m_decoder) {
			if (m_state.load(std::memory_order_relaxed) == VoiceState::Destroyed) {
				SIREN_LOG_ERROR("Voice::playOneShot() Unable to play destroyed voice");
				return;
			}

			m_isReusable.store(false, std::memory_order_relaxed);
			m_isLooping.store(false, std::memory_order_relaxed);
			m_snapGainRequested.store(true, std::memory_order_relaxed);
			m_seekFrame.store(0, std::memory_order_relaxed);
			m_state.store(VoiceState::Playing, std::memory_order_release);
		}
	}

	bool Voice::isPlaying() const {
		return m_state.load(std::memory_order_relaxed) == VoiceState::Playing;
	}

	void Voice::pause() {
		m_state.store(VoiceState::Paused, std::memory_order_release);
	}

	bool Voice::isPaused() const {
		return m_state.load(std::memory_order_relaxed) == VoiceState::Paused;
	}

	void Voice::stop() {
		if (!m_isReusable.load(std::memory_order_relaxed)) {
			m_state.store(VoiceState::Destroyed);
		}
		else {
			if (m_decoder) m_seekFrame.store(0, std::memory_order_relaxed);
			m_state.store(VoiceState::Inactive, std::memory_order_release);
		}
	}

	bool Voice::isFinished() const {
		VoiceState state = m_state.load(std::memory_order_relaxed);
		return state == VoiceState::Inactive || state == VoiceState::Destroyed;
	}

	void Voice::seek(float timePoint) {
		// TODO: Check if timePoint is out of bounds
		// This will require that we store totalFrames in voice as a memeber variable
		if (m_state.load(std::memory_order_relaxed) == VoiceState::Destroyed) {
			return;
		}
		int64_t frame = static_cast<int64_t>(timePoint * m_sampleRate);
		m_seekFrame.store(frame, std::memory_order_relaxed);
	}

	void Voice::destroy() {
		m_state.store(VoiceState::Destroyed, std::memory_order_seq_cst);

		while (m_isMixing.load(std::memory_order_acquire)) {
			std::this_thread::yield();
		}
	}

	void Voice::setReusable(bool value) {
		m_isReusable.store(value, std::memory_order_relaxed);
	}

	bool Voice::isReusable() const {
		return m_isReusable.load(std::memory_order_relaxed);
	}

	void Voice::setGlobal() {
		m_mode = VoiceMode::Global;
		m_pan = 0.0f; // Reset pan
	}

	void Voice::setBus(std::shared_ptr<AudioBus> bus) {
		m_bus.store(bus, std::memory_order_release);
	}

	void Voice::setVolume(float value) {
		m_volume = std::clamp(value, 0.0f, 1.0f);
	}

	float Voice::getVolume() const {
		return m_volume;
	}

	void Voice::setPan(float value) {
		m_pan = std::clamp(value, -1.0f, 1.0f);
	}

	float Voice::getPan() const {
		return m_pan;
	}

	void Voice::setLooping(bool value) {
		m_isLooping.store(value, std::memory_order_relaxed);
	}

	bool Voice::isLooping() const {
		return m_isLooping.load(std::memory_order_relaxed);
	}

	void Voice::setPitch(float value) {
		m_pitch.store(std::clamp(value, 0.1f, 4.0f), std::memory_order_relaxed);
	}

	float Voice::getPitch() const {
		return m_pitch.load(std::memory_order_relaxed);
	}

	void Voice::setDopplerEffect(bool value) {
		m_dopplerEffect = value;
	}

	void Voice::setDopplerFactor(float value) {
		m_dopplerFactor = value;
	}

	float Voice::getDopplerFactor() const {
		return m_dopplerFactor;
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

	void Voice::setVelocitySmoothing(float value) {
		m_velocitySmoothing = value;
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

	void Voice::setTag(const std::string& tag) {
		m_tag = tag;
	}

	const std::string& Voice::getTag() {
		return m_tag;
	}

	void Voice::attachDecoder(std::unique_ptr<Decoder> decoder) {
		m_sampleRate = decoder->getSampleRate();
		m_decoder = std::move(decoder);
	}

	bool Voice::prepare() {
		if (!m_decoder) {
			SIREN_LOG_ERROR("Voice::prepare() Called without a decoder attatched")
			return false;
		}

		m_resampler.init(m_decoder->getChannelCount());
		
		if (m_decoder->seek(0) != ResultCode::Success) {
			SIREN_LOG_ERROR("Voice::prepare() Failed to seek to beginning");
			return false;
		}

		m_isReusable.store(true);
		m_isLooping.store(false);

		m_pitch.store(1.0f);
		m_dopplerPitch.store(1.0f);

		m_targetGainL.store(1.0f);
		m_targetGainR.store(1.0f);
		m_snapGainRequested.store(true, std::memory_order_relaxed);

		m_state.store(VoiceState::Inactive);

		return true;
	}

	bool Voice::mix() noexcept {
		if (m_state.load(std::memory_order_relaxed) == VoiceState::Destroyed) {
			return false; // Dead
		}

		// Flag the main thread that voice is mixing and voice can not be destroyed until done
		m_isMixing.store(true, std::memory_order_seq_cst);
		if (m_state.load(std::memory_order_seq_cst) == VoiceState::Destroyed) {
			m_isMixing.store(false, std::memory_order_acquire);
			return false;
		}

		// -- PROTECTED BLOCK START --
		bool stillAlive = true;
		{
			VoiceState state = m_state.load(std::memory_order_relaxed);

			if (state == VoiceState::Inactive || state == VoiceState::Paused) {
				m_isMixing.store(false, std::memory_order_release);
				return true; // Still alive but not mixed
			}

			std::shared_ptr<AudioBus> bus = m_bus.load(std::memory_order_acquire);
			if (!m_decoder || !bus) {
				m_isMixing.store(false, std::memory_order_release);
				return false; // Dead
			}

			// Handle seek requests
			int64_t seekRequest = m_seekFrame.exchange(-1);
			if (seekRequest >= 0 && m_decoder) {
				if (m_decoder->seek(static_cast<size_t>(seekRequest)) == ResultCode::Success) {
					m_resampler.flush();
				}
			}

			std::span<float> dst = bus->m_buffer;
			constexpr size_t BUFFER_FRAMES = 256;
			// TODO: This 2 represents the maximum number of channels and should be a constant like MAX_CHANNELS
			std::array<float, BUFFER_FRAMES * 2> intermediateBuffer;


			size_t decoderChannels = m_decoder->getChannelCount();
			float pitch = m_pitch.load(std::memory_order_relaxed) * m_dopplerPitch.load(std::memory_order_relaxed);
			bool isLooping = m_isLooping.load(std::memory_order_relaxed);
			float targetGainL = m_targetGainL.load(std::memory_order_relaxed);
			float targetGainR = m_targetGainR.load(std::memory_order_relaxed);

			if (m_snapGainRequested.exchange(false, std::memory_order_relaxed)) {
				m_currentGainL = targetGainL;
				m_currentGainR = targetGainR;
			}

			bool continuePlayback = true; // Lambda sets this to false if EOF is hit and voice is not looping or on error

			auto dataProvider = [&](std::span<float> buffer) -> size_t {
				size_t totalSamplesRead = 0;
				size_t totalSamplesRequested = buffer.size();
				size_t totalFramesRead = 0;
				size_t channels = decoderChannels;

				while (totalSamplesRead < totalSamplesRequested) {
					std::span<float> subBuffer = buffer.subspan(totalSamplesRead);
					if (subBuffer.size() < channels) {
						break; // Alignment guard (for invalid audio data)
					}
					size_t framesJustRead = m_decoder->decode(subBuffer);
					size_t samplesJustRead = framesJustRead * channels;

					totalSamplesRead += samplesJustRead;
					totalFramesRead += framesJustRead;

					if (framesJustRead == 0) {
						if (isLooping) {
							if (m_decoder->seek(0) != ResultCode::Success) {
								continuePlayback = false;
								break;
							}
						}
						else {
							continuePlayback = false;
							break;
						}
					}
				}

				return totalFramesRead;
			};

			// TODO: This 2 represents output channels and should probably be aquired from the output bus
			size_t framesRequested = dst.size() / 2;
			size_t framesRead = 0;

			const float SLEW_RATE = 0.0002f;

			while (framesRead < framesRequested) {

				size_t framesToDecode = std::min(framesRequested - framesRead, BUFFER_FRAMES);
				std::span<float> resamplerOutput(intermediateBuffer.data(), framesToDecode * decoderChannels);

				size_t framesDecoded = m_resampler.getSamples(resamplerOutput, pitch, dataProvider);

				// Mix and add to destination buffer
				for (size_t i = 0; i < framesDecoded; i++) {
					float sampleL;
					float sampleR;

					if (decoderChannels == 1) {
						// Mono
						sampleL = intermediateBuffer[i];
						sampleR = intermediateBuffer[i];
					}
					else {
						// Stereo
						sampleL = intermediateBuffer[i * 2];
						sampleR = intermediateBuffer[i * 2 + 1];
					}

					float diffL = targetGainL - m_currentGainL;
					if (std::abs(diffL) < SLEW_RATE) {
						m_currentGainL = targetGainL;
					}
					else {
						m_currentGainL += (diffL > 0) ? SLEW_RATE : -SLEW_RATE;
					}

					float diffR = targetGainR - m_currentGainR;
					if (std::abs(diffR) < SLEW_RATE) {
						m_currentGainR = targetGainR;
					}
					else {
						m_currentGainR += (diffR > 0) ? SLEW_RATE : -SLEW_RATE;
					}

					size_t dstIndex = (framesRead + i) * 2;
					dst[dstIndex] += sampleL * m_currentGainL;
					dst[dstIndex + 1] += sampleR * m_currentGainR;
				}

				framesRead += framesDecoded;

				if (m_state.load(std::memory_order_relaxed) != VoiceState::Playing) {
					break;
				}

				if (!continuePlayback && framesDecoded < framesToDecode) {
					if (!m_isReusable.load(std::memory_order_relaxed)) {
						m_state.store(VoiceState::Destroyed, std::memory_order_release);
						stillAlive = false;
					}
					else {
						m_state.store(VoiceState::Inactive, std::memory_order_release);
					}
					break;
				}
			}

			if (m_state.load(std::memory_order_relaxed) == VoiceState::Destroyed) {
				stillAlive = false;
			}
		}

		m_isMixing.store(false, std::memory_order_release);

		return stillAlive;
	}

	void Voice::update(float deltaTime, const ListenerData& listener, float globalDopplerScale) {
		float volume = m_volume;
		float pan = m_pan;

		if (m_mode == VoiceMode::Spatial) {
			// DISTANCE BASED VOLUME
			// TODO: Use inverse square law to calculate realistic volume dropoff
			Vector3 listenerToEmitter = m_position - listener.position;
			float distance = (m_position - listener.position).length();
			float distanceClamped = std::clamp(distance, m_minDistance, m_maxDistance);
			float fraction = (distanceClamped - m_minDistance) / (m_maxDistance - m_minDistance);
			volume = volume - fraction;

			// SPATIAL PANNING
			Vector3 listenerToEmitterNormalized = normalize(listenerToEmitter);
			Vector3 right = listener.right;
			pan = right * listenerToEmitterNormalized;

			// VELOCITY APPROXIMATION
			if (m_velocitySetThisFrame) {
				// Velocity has been manually overridden and does not need to be calculated
				m_velocitySetThisFrame = false;
			}
			else if (m_firstUpdate) {
				// First update -> don't calculate velocity
				m_velocity = { 0.0f, 0.0f, 0.0f };
			}
			else {
				if (deltaTime > 0.00001) {
					// Approximate voice velocity
					Vector3 distanceVec = m_position - m_previousPosition;
					Vector3 rawVelocity = distanceVec / deltaTime;

					// Exponential smoothing of velocity
					float smoothingFactor = std::clamp(deltaTime * m_velocitySmoothing, 0.0f, 1.0f);
					m_velocity = m_velocity + (rawVelocity - m_velocity) * smoothingFactor;
				}
			}

			m_firstUpdate = false;
			m_previousPosition = m_position;

			// DOPPLER EFFECT
			if (m_dopplerEffect) {
				// Doppler pitch calculation (Relative Velocity Projection Formula)
				if (distance < 0.001f) {
					m_dopplerPitch.store(1.0f, std::memory_order_relaxed);
				}
				else {
					// Project velocites onto listenerToVoice
					float listenerVel = listener.velocity * listenerToEmitterNormalized;
					float emitterVel = m_velocity * listenerToEmitterNormalized;

					float dopplerStrenght = m_dopplerFactor * globalDopplerScale;
					float numerator = SPEED_OF_SOUND + (listenerVel * dopplerStrenght);
					float denominator = SPEED_OF_SOUND + (emitterVel * dopplerStrenght);
					float dopplerPitch = numerator / std::max(denominator, 0.1f);
					m_dopplerPitch.store(std::clamp(dopplerPitch, 0.1f, 4.0f), std::memory_order_relaxed);
				}
			}
			else {
				m_dopplerPitch.store(1.0f, std::memory_order_relaxed);
			}
		}

		float panNormalized = (pan + 1.0f) * 0.5f;
		float angle = panNormalized * static_cast<float>(M_PI_2); // Angle between 0 and PI/2 radians

		// Apply pan and volume
		float gainL = std::cos(angle) * volume;
		float gainR = std::sin(angle) * volume;
		m_targetGainL.store(gainL, std::memory_order_relaxed);
		m_targetGainR.store(gainR, std::memory_order_relaxed);
	}

	Voice::VoiceState Voice::getState() {
		return m_state.load(std::memory_order_relaxed);
	}
}
