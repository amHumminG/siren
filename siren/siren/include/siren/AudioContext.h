#pragma once
#include "siren/Voice.h"
#include "siren/Sound.h"
#include <vector>
#include <mutex>

struct ma_device;

namespace siren {

	class AudioContext {
	private:
		bool m_initialized = false;
		std::unique_ptr<ma_device> m_device;

		struct PendingVoiceNode {
			std::shared_ptr<Voice> voice;
			PendingVoiceNode* next = nullptr;
		};

		std::atomic<PendingVoiceNode*> m_inboxHead{ nullptr }; // Linked list of pending voices to be played

		std::vector<std::shared_ptr<Voice>> m_voiceRegistry; // Only accessed by data_callback

		/// @brief Writes audio data to the device
		/// @param pDevice The device
		/// @param pOutput Buffer for the audio data to be written to
		/// @param pInput Unused (used for audio recording)
		/// @param frameCount The number of frames requested by the audio device
		static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, uint32_t frameCount);

	public:
		AudioContext();
		~AudioContext();

		/// @brief Initializes the AudioContext
		/// @return True if intialization was successful, otherwise false
		bool init();

		/// @brief Deinitializes the AudioContext
		/// @return True if deinitialization was successful, otherwise false
		bool deinit();

		/// @brief Plays a sound
		/// @param sound The sound to be played
		/// @return A shared pointer to the voice that has been created to play the sound
		std::shared_ptr<Voice> play(const Sound& sound);
	};
}