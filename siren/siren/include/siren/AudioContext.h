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

		static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, uint32_t frameCount);

	public:
		AudioContext();
		~AudioContext();

		bool init();
		bool deinit();

		std::shared_ptr<Voice> play(const Sound& sound);
	};
}