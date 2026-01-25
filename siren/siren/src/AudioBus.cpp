#include "siren/AudioBus.h"
#include <algorithm>

namespace siren {

	AudioBus::AudioBus() {
		m_buffer.reserve(MAX_BUFFER_SIZE);
	}

	void AudioBus::prepare(size_t frameCount, size_t channelCount) {
		size_t requestedSize = frameCount * channelCount;

		// TODO: This should be removed to adhere to guidelines
		if (requestedSize > m_buffer.capacity()) {
			// Worst case scenario -> Allocate more memory (this should never happen)
			m_buffer.resize(requestedSize);
		}
		else {
			m_buffer.resize(requestedSize);
		}

		std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
	}

	void AudioBus::process() {
		float targetGain = m_volume.load();
		const float SLEW_RATE = 0.0002f;

		for (size_t i = 0; i < m_buffer.size(); i += 2) {
			float diff = targetGain - m_currentGain;
			if (std::abs(diff) < SLEW_RATE) {
				m_currentGain = targetGain;
			}
			else {
				m_currentGain += (diff > 0) ? SLEW_RATE : -SLEW_RATE;
			}

			m_buffer[i]		*= m_currentGain;
			m_buffer[i + 1]	*= m_currentGain;
		}
	}

	void AudioBus::setVolume(float value) {
		m_volume.store(std::clamp(value, 0.0f, 1.0f), std::memory_order_relaxed);
	}

	float AudioBus::getVolume() {
		return m_volume.load(std::memory_order_relaxed);
	}
}