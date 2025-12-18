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
		std::mutex m_mutex;
		std::vector<std::shared_ptr<Voice>> m_voiceRegistry;

		static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

	public:
		AudioContext();
		~AudioContext();

		bool init();
		bool deinit();

		std::shared_ptr<Voice> play(const Sound& sound);
	};
}