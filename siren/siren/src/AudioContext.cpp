#include "siren/AudioContext.h"
#include "internal/log.h"
#include "siren/DecoderFactory.h"

#define MINIAUDIO_IMPLEMENTATION
#include "../external/miniaudio/miniaudio.h"


namespace siren {

	void AudioContext::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, uint32_t frameCount) {
		AudioContext* context = static_cast<AudioContext*>(pDevice->pUserData);
		if (!context) {
			return;
		}

		float* outBuffer = static_cast<float*>(pOutput);
		size_t channelCount = static_cast<size_t>(pDevice->playback.channels);

		// Prepare final output buffer and bus buffers
		std::fill_n(outBuffer, frameCount * channelCount, 0.0f); // Silence baseline
		context->m_sfxBus.prepare(frameCount, channelCount);
		context->m_musicBus.prepare(frameCount, channelCount);

		// Process pending voices
		PendingVoiceNode* rawList = context->m_inboxHead.exchange(nullptr); // Get inbox
		while (rawList != nullptr) {
			std::unique_ptr<PendingVoiceNode> node(rawList);
			rawList = node->next;
			context->m_voiceRegistry.push_back(std::move(node->voice));
		}

		auto& voices = context->m_voiceRegistry;
		for (auto it = voices.begin(); it != voices.end(); ) {
			auto& voice = *it;

			bool alive = voice->mix();
			if (!alive) {
				it = voices.erase(it);
			}
			else {
				it++;
			}
		}

		float sfxVolume = context->m_sfxBus.m_volume;
		for (size_t i = 0; i < context->m_sfxBus.m_buffer.size(); i++) {
			outBuffer[i] += context->m_sfxBus.m_buffer[i] * sfxVolume;
		}

		float musicVolume = context->m_musicBus.m_volume;
		for (size_t i = 0; i < context->m_musicBus.m_buffer.size(); i++) {
			outBuffer[i] += context->m_musicBus.m_buffer[i] * musicVolume;
		}

		// TODO: Add master volume
	}

	AudioContext::AudioContext() {
		m_device = std::make_unique<ma_device>();
	}

	AudioContext::~AudioContext() {
		deinit();

		// Clean up voice inbox
		PendingVoiceNode* node = m_inboxHead.exchange(nullptr);
		while (node != nullptr) {
			PendingVoiceNode* next = node->next;
			delete node;
			node = next;
		}
	}

	bool AudioContext::init() {
		// Configure device
		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = ma_format_f32;
		config.playback.channels = 2; // Only supports stereo playback for now
		config.sampleRate = 44100; // Only supports 44100 Hz
		config.dataCallback = data_callback;
		config.pUserData = this;

		// Initialize device
		if (ma_device_init(NULL, &config, m_device.get()) != MA_SUCCESS) {
			SIREN_LOG_ERROR("AudioContext::init() Failed to initialize device");
			return false;
		}

		// Start device
		if (ma_device_start(m_device.get()) != MA_SUCCESS) {
			SIREN_LOG_ERROR("AudioContext::init() Failed to start device");
			return false;
		}

		m_initialized = true;
		SIREN_LOG_INFO("AudioContext: Initialized (44100Hz Stereo)");
		return true;
	}

	bool AudioContext::deinit() {
		if (!m_initialized) {
			return true;
		}

		if (ma_device_stop(m_device.get()) != MA_SUCCESS) {
			SIREN_LOG_ERROR("AudioContext::deinit() Failed to stop device");
			return false;
		}

		ma_device_uninit(m_device.get());
		m_initialized = false;
		SIREN_LOG_INFO("AudioContext: Uninitialized")
		return true;
	}

	std::shared_ptr<Voice> AudioContext::play(const Sound& sound) {
		// Create voice with a decoder
		if (!sound.isValid()) {
			SIREN_LOG_ERROR("AudioContext::play() Invalid sound");
			return nullptr;
		}
		auto voice = std::make_shared<Voice>();
		uint32_t sampleRate = 0;
		size_t totalFrames = 0;
		if (sound.getType() == SoundType::Stream) {
			Result result = DecoderFactory::createDecoder(sound.getPath());
			if (!result.isOk()) {
				SIREN_LOG_ERROR("AudioContext::play() Failed to create decoder. ERROR: " << int(result.error()));
				return nullptr;
			}
			sampleRate = result.value()->getSampleRate();
			totalFrames = result.value()->getTotalFrames();
			voice->attachDecoder(std::move(result.value()));
		}
		else { // Sound type == MemoryInternal or MemoryExternal
			Result result = DecoderFactory::createDecoder(sound.getData());
			if (!result.isOk()) {
				SIREN_LOG_ERROR("AudioContext::play() Failed to create decoder. ERROR: " << int(result.error()));
				return nullptr;
			}
			sampleRate = result.value()->getSampleRate();
			totalFrames = result.value()->getTotalFrames();
			voice->attachDecoder(std::move(result.value()));
		}
		voice->setTag(sound.getTag());
		voice->setBus(&m_musicBus); // TODO: make this dynamic
		voice->play();

		size_t seconds = totalFrames / sampleRate;
		SIREN_LOG_INFO("Playing sound [" << voice->getTag() << ", " << seconds / 60 << "m " << seconds % 60 << "s]");

		auto newNode = std::make_unique<PendingVoiceNode>();
		newNode->voice = voice;

		PendingVoiceNode* rawNode = newNode.get();
		PendingVoiceNode* head = m_inboxHead.load();
		
		do {
			rawNode->next = head;
		} while (!m_inboxHead.compare_exchange_weak(head, rawNode));

		newNode.release(); // Pointer is now owned by inbox
		return voice; 
	}
}