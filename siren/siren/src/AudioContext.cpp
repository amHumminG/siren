#include "siren/AudioContext.h"
#include "internal/log.h"
#include "siren/DecoderFactory.h"
#include <algorithm>

#define MINIAUDIO_IMPLEMENTATION
#include "../external/miniaudio/miniaudio.h"


namespace siren {

	void AudioContext::data_callback(ma_device* pDevice, void* pOutput, const void* pInput, uint32_t frameCount) {
		AudioContext* context = static_cast<AudioContext*>(pDevice->pUserData);
		if (!context) return;

		float* outBuffer = static_cast<float*>(pOutput);
		size_t channelCount = static_cast<size_t>(pDevice->playback.channels);
		size_t requestedSamples = frameCount * channelCount;

		// Prepare final output buffer and bus buffers
		std::fill_n(outBuffer, requestedSamples, 0.0f); // Silence baseline

		// Handle flush request
		if (context->m_flushRequested.load(std::memory_order_acquire)) {
			context->m_voicesAudio.clear();

			context->m_flushRequested.store(false, std::memory_order_release);
			context->m_flushCompleted.store(true, std::memory_order_release);

			return;
		}

		// Process pending voices
		PendingVoiceNode* inbox = context->m_inboxHead.exchange(nullptr, std::memory_order_acq_rel); // Get inbox
		while (inbox) {
			std::unique_ptr<PendingVoiceNode> node(inbox);
			inbox = node->next;
			context->m_voicesAudio.push_back(std::move(node->voice));
		}

		std::shared_lock<std::shared_mutex> lock(context->m_busMutex);
		for (auto& [name, bus] : context->m_busRegistry) {
			bus->prepare(frameCount, channelCount);
		}

		auto& voices = context->m_voicesAudio;
		for (auto it = voices.begin(); it != voices.end(); ) {
			auto& voice = *it;

			bool alive = voice->mix(); // Mix voice into bus buffer
			if (!alive) {
				it = voices.erase(it); // TODO: Queue deletion to be done in update()
			}
			else {
				it++;
			}
		}
				
		AudioBus* masterBus = context->m_cachedMasterBus;
		if (!masterBus) return;

		for (auto& [name, bus] : context->m_busRegistry) {
			if (bus.get() != masterBus) {
				bus->process();
				size_t limit = (std::min)(bus->m_buffer.size(), masterBus->m_buffer.size());
				for (size_t i = 0; i < limit; i++) {
					masterBus->m_buffer[i] += bus->m_buffer[i];
				}
			}
		}

		masterBus->process();
		for (size_t i = 0; i < requestedSamples; i++) {
			outBuffer[i] = std::clamp(masterBus->m_buffer[i], -1.0f, 1.0f);
		}
	}

	AudioBus* AudioContext::getBus(const std::string& busName) noexcept {
		std::shared_lock<std::shared_mutex> lock(m_busMutex);

		auto it = m_busRegistry.find(busName);
		if (it == m_busRegistry.end()) {
			return nullptr;
		}

		return it->second.get();
	}

	AudioContext::AudioContext() {
		m_device = std::make_unique<ma_device>();
		m_busRegistry["Master"] = std::make_unique<AudioBus>();
		m_cachedMasterBus = m_busRegistry["Master"].get();
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

	void AudioContext::update(float deltaTime) {
		ListenerData listener;
		{
			std::lock_guard<std::mutex> lock(m_listenerMutex);
			listener = m_listener;
		}

		if (m_listenerVelocitySetThisFrame) {
			m_listenerVelocitySetThisFrame = false;
		}
		else {
			if (deltaTime > 0.00001) {
				// Approximate listener velocity
				Vector3 distance = listener.position - m_previousListenerPos;
				Vector3 rawVelocity = distance / deltaTime;

				// Exponential smoothing of listener velocity
				float smoothingFactor = std::clamp(deltaTime * m_listenerVelocitySmoothing, 0.0f, 1.0f);
				listener.velocity = listener.velocity + (rawVelocity - listener.velocity) * smoothingFactor;

				// Update listener with approximated velocity
				{
					std::lock_guard<std::mutex> lock(m_listenerMutex);
					m_listener.velocity = listener.velocity;
				}
			}
		}

		// Voice updates
		for (auto it = m_voicesMain.begin(); it != m_voicesMain.end(); ) {
			auto& voice = *it;

			if (voice->getState() == VoiceState::Dead) {
				it = m_voicesMain.erase(it);
			}
			else {
				voice->update(deltaTime, listener, m_globalDopplerScale);
				it++;
			}
		}

		m_previousListenerPos = listener.position;
	}

	void AudioContext::flush() {
		m_voicesMain.clear();

		PendingVoiceNode* node = m_inboxHead.exchange(nullptr, std::memory_order_acquire);
		while (node) {
			PendingVoiceNode* next = node->next;
			delete node;
			node = next;
		}

		// Request flush from audio thread
		m_flushCompleted.store(false, std::memory_order_release);
		m_flushRequested.store(true, std::memory_order_release);

		// Wait for flush to finish
		while (!m_flushCompleted.load(std::memory_order_acquire)) {
			std::this_thread::yield();
		}
	}

	void AudioContext::setCoordinateSystem(CoordinateSystem system) {
		m_coordinateSystem = system;
	}

	void AudioContext::setListener(const Vector3& pos, const Vector3& fwd, const Vector3& up) {
		setListenerPos(pos);
		setListenerOrientation(fwd, up);
	}

	void AudioContext::setListenerPos(const Vector3& pos) {
		std::lock_guard<std::mutex> lock(m_listenerMutex);

		m_listener.position = pos;
	}

	void AudioContext::setListenerOrientation(const Vector3& fwd, const Vector3& up) {
		std::lock_guard<std::mutex> lock(m_listenerMutex);

		m_listener.forward = normalize(fwd);
		m_listener.up = normalize(up);

		if (m_coordinateSystem == CoordinateSystem::RightHanded) {
			m_listener.right = normalize(crossMultiply(m_listener.forward, m_listener.up));
		}
		else {
			m_listener.right = normalize(crossMultiply(m_listener.up, m_listener.forward));
		}
	}

	void AudioContext::setListenerVelocity(const Vector3& vel) {
		std::lock_guard<std::mutex> lock(m_listenerMutex);
		m_listener.velocity = vel;
		m_listenerVelocitySetThisFrame = true;
	}

	void AudioContext::setListenerVelocitySmoothing(float value) {
		m_listenerVelocitySmoothing = value;
	}

	void AudioContext::setDefaultVoiceVelocitySmoothing(float value) {
		m_defaultVoiceVelocitySmoothing = value;
	}

	void AudioContext::setGlobalDopplerScale(float value) {
		m_globalDopplerScale = value;
	}

	float AudioContext::getGlobalDopplerScale() const {
		return m_globalDopplerScale;
	}

	ListenerData AudioContext::getListener() const {
		std::lock_guard<std::mutex> lock(m_listenerMutex);
		return m_listener;
	}

	bool AudioContext::createBus(const std::string& busName) {
		std::unique_lock<std::shared_mutex> lock(m_busMutex);

		auto it = m_busRegistry.find(busName);
		if (it != m_busRegistry.end()) {
			SIREN_LOG_ERROR("AudioContext::createBus() Bus with name: " << busName << " already exists");
			return false;
		}

		auto bus = std::make_unique<AudioBus>();
		m_busRegistry[busName] = std::move(bus);

		return true;
	}

	bool AudioContext::setBusVolume(const std::string& busName, float volume) {
		AudioBus* bus = getBus(busName);
		if (bus == nullptr) {
			SIREN_LOG_WARNING("AudioContext::setBusVolume() No bus with name: " << busName << " exists");
			return false;
		}

		bus->m_volume.store(std::clamp(volume, 0.0f, 1.0f));
		return true;
	}

	float AudioContext::getBusVolume(const std::string& busName) {
		AudioBus* bus = getBus(busName);
		if (!bus) {
			SIREN_LOG_WARNING("AudioContext::getBusVolume() No bus with name: " << busName << " exists");
			return 0.0f;
		}

		return bus->m_volume;
	}

	std::shared_ptr<Voice> AudioContext::createVoice(const Sound& sound, const std::string& busName) {
		if (!sound.isValid()) {
			SIREN_LOG_ERROR("AudioContext::createVoice() Invalid sound");
			return nullptr;
		}

		// Create data source
		Result sourceResult = sound.createDataSource();
		if (!sourceResult.isOk()) {
			SIREN_LOG_ERROR("AudioContext::createVoice() Failed to create data source. ERROR: " << int(sourceResult.error()));
			return nullptr;
		}

		// Create decoder for that data source
		Result decoderResult = DecoderFactory::createDecoder(std::move(sourceResult.value()));
		if (!decoderResult.isOk()) {
			SIREN_LOG_ERROR("AudioContext::createVoice() Failed to create decoder. ERROR: " << int(decoderResult.error()));
			return nullptr;
		}

		auto voice = std::make_shared<Voice>();

		uint32_t sampleRate = decoderResult.value()->getSampleRate();
		size_t totalFrames = decoderResult.value()->getTotalFrames();

		voice->attachDecoder(std::move(decoderResult.value()));
		voice->setTag(sound.getTag());
		voice->setVelocitySmoothing(m_defaultVoiceVelocitySmoothing);
		if (!voice->prepare()) {
			SIREN_LOG_ERROR("AudioContext::createVoice() Failed to prepare voice");
			return nullptr;
		}

		AudioBus* bus = getBus(busName);
		if (bus == nullptr) {
			SIREN_LOG_WARNING("AudioContext::createVoice() No bus with name: " << busName << " exists. Defaulting to Master");
			bus = getBus("Master");
		}
		if (bus == nullptr) { // Default to master bus if bus was not found
			SIREN_LOG_ERROR("AudioContext::createVoice() Master bus does not exist");
			return nullptr;
		}
		voice->setBus(bus);

		// DEBUG
		size_t seconds = totalFrames / sampleRate;
		SIREN_LOG_INFO("Created sound [" << voice->getTag() << ", " << seconds / 60 << "m " << seconds % 60 << "s]");

		m_voicesMain.push_back(voice);

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