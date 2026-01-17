#include "Resampler.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace siren {

	void Resampler::init(uint16_t channelCount) {
		m_channels = channelCount;
		flush();

		size_t maxFramesNeeded = static_cast<size_t>(MAX_BLOCK_SIZE * MAX_PITCH) + PADDING;
		m_inputBuffer.assign(maxFramesNeeded * m_channels, 0.0f);
	}

	void Resampler::flush() {
		m_cursor = 0.0f;
		m_validInputFrames = 0;
	}

	size_t Resampler::getSamples(std::span<float> dst, float pitch, 
		std::function<size_t(std::span<float>)> dataProvider) {
		if (m_channels == 0) {
			return 0;
		}

		float safePitch = std::clamp(pitch, 0.1f, MAX_PITCH);

		size_t dstFramesRequested = dst.size() / m_channels;
		if (dstFramesRequested > MAX_BLOCK_SIZE) {
			// Maybe use an assert here
			return 0;
		}

		float inputFrameRequestedFloat = (dstFramesRequested * safePitch) + PADDING;
		size_t inputFramesRequested = static_cast<size_t>(std::ceil(inputFrameRequestedFloat));

		size_t inputBufferFrameCapacity = m_inputBuffer.size() / m_channels;
		if (inputFramesRequested > inputBufferFrameCapacity) {
			inputFramesRequested = inputBufferFrameCapacity; // Clamp to prevent issues (Temporary solution)
		}

		// Fill buffer if needed
		if (m_validInputFrames < inputFramesRequested) {
			float* writePtr = m_inputBuffer.data() + (m_validInputFrames * m_channels);
			size_t availableInputFrames = inputBufferFrameCapacity - m_validInputFrames;
			size_t inputFramesToFill = std::min(inputFramesRequested - m_validInputFrames, availableInputFrames);

			if (inputFramesToFill > 0) {
				std::span<float> writeDst(writePtr, inputFramesToFill * m_channels);
				size_t framesRead = dataProvider(writeDst);
				m_validInputFrames += framesRead;
			}
		}

		// Resample
		size_t dstFramesGenerated = 0;
		size_t dstIndex = 0;

		while (dstFramesGenerated < dstFramesRequested) {
			size_t index0 = static_cast<size_t>(m_cursor);
			size_t index1 = index0 + 1;

			if (index1 >= m_validInputFrames) { // No more data in input buffer
				break;
			}

			float alpha = m_cursor - index0;
			for (int c = 0; c < m_channels; c++) {
				float sampleA = m_inputBuffer[index0 * m_channels + c];
				float sampleB = m_inputBuffer[index1 * m_channels + c];

				dst[dstIndex++] = std::lerp(sampleA, sampleB, alpha);
			}

			m_cursor += safePitch;
			dstFramesGenerated++;
		}

		// Shift leftover frames
		size_t consumedInputFrames = static_cast<size_t>(m_cursor);
		if (consumedInputFrames > 0) {
			size_t remaining = (consumedInputFrames < m_validInputFrames) ? (m_validInputFrames - consumedInputFrames) : 0;
			if (remaining > 0) {
				std::memmove(
					m_inputBuffer.data(),
					m_inputBuffer.data() + (consumedInputFrames * m_channels),
					remaining * m_channels * sizeof(float));
			}

			m_validInputFrames = remaining;
			m_cursor -= consumedInputFrames;
		}

		return dstFramesGenerated;
	}


}