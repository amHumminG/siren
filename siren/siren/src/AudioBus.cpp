#include "siren/AudioBus.h"
#include <algorithm>

namespace siren {

	AudioBus::AudioBus() {
		m_buffer.reserve(MAX_BUFFER_SIZE);
	}

	void AudioBus::prepare(size_t frameCount, size_t channelCount) {
		size_t requestedSize = frameCount * channelCount;

		if (requestedSize > m_buffer.capacity()) {
			// Worst case scenario -> Allocate more memory (this should never happen)
			m_buffer.resize(requestedSize);
		}
		else {
			m_buffer.resize(requestedSize);
		}

		std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
	}

	void AudioBus::setVolume(float value) {
		m_volume.store(std::clamp(value, 0.0f, 1.0f));
	}


}