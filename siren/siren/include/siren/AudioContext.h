#pragma once
#include "siren/AudioBus.h"
#include "siren/Voice.h"
#include "siren/Sound.h"
#include "siren/SirenMath.h"
#include <vector>
#include <unordered_map>
#include <shared_mutex>

enum class CoordinateSystem {
	RightHanded,
	LeftHanded
};

struct ma_device;

namespace siren {

	class AudioContext {
	private:
		bool m_initialized = false;
		std::unique_ptr<ma_device> m_device; // The device used for audio playback

		std::unordered_map<std::string, std::unique_ptr<AudioBus>> m_busRegistry; // Contains all audio buses
		std::shared_mutex m_busMutex;

		struct PendingVoiceNode {
			std::shared_ptr<Voice> voice;
			PendingVoiceNode* next = nullptr;
		};

		std::atomic<PendingVoiceNode*> m_inboxHead{ nullptr }; // Linked list of pending voices to be played

		std::vector<std::shared_ptr<Voice>> m_voiceRegistry; // Only accessed by data_callback
		
		CoordinateSystem m_coordinateSystem = CoordinateSystem::LeftHanded;
		ListenerData m_listener; // Represents the listener (most likely the player)
		Vector3 m_previousListenerPos;
		bool m_listenerVelocitySetThisFrame = false;
		mutable std::mutex m_listenerMutex; // Mutable for use in getListener()

		/// @brief Writes audio data to the device
		/// @param pDevice The device
		/// @param pOutput Buffer for the audio data to be written to
		/// @param pInput Unused (used for audio recording)
		/// @param frameCount The number of frames requested by the audio device
		static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, uint32_t frameCount);

		/// @param busName The name of the bus
		/// @return A pointer to the audio bus or nullptr if no bus 
		/// with the specified name exist
		AudioBus* getBus(const std::string& busName);

	public:
		AudioContext();
		~AudioContext();

		/// @brief Initializes the AudioContext
		/// @return True if intialization was successful, otherwise false
		bool init();

		/// @brief Deinitializes the AudioContext
		/// @return True if deinitialization was successful, otherwise false
		bool deinit();

		/// @brief Central update function to be called each frame.
		/// Handles all audio logic
		/// @param deltaTime Frame time difference
		void update(float deltaTime);

		/// @brief Sets the orientation of the coordinatesystem
		void setCoordinateSystem(CoordinateSystem system);

		/// @brief Sets the listener data
		/// @param pos The position of the listener
		/// @param fwd The direction the listener is facing
		/// @param up The up vector from the listener
		void setListener(const Vector3& pos, const Vector3& fwd, const Vector3& up);

		/// @brief Sets the listener position
		void setListenerPos(const Vector3& pos);

		/// @brief Sets the listener orientation
		/// @param fwd The direction the listener is facing
		/// @param up The up vector from the listener
		void setListenerOrientation(const Vector3& fwd, const Vector3& up);

		/// @brief Manual velocity override 
		/// (Use this if you have velocity data to avoid engine approximation)
		void setListenerVelocity(const Vector3& vel);

		/// @return All listener data currently stored in the context
		///
		/// Note that if no velocity was provided for a frame, it will be approximated
		/// by the engine
		ListenerData getListener() const;

		/// @brief Creates a bus with the specified name
		/// @param busName The name of the bus
		/// @return True if the bus was created, otherwise false
		bool createBus(const std::string& busName);

		/// @param volume New volume (clamped between 0.0f and 1.0f)
		bool setBusVolume(const std::string& busName, float volume);

		/// @brief Returns the volume of a specified bus
		/// 
		/// If no bus with the specified name exists, return will be 0.0f
		/// @param busName The name of the bus
		float getBusVolume(const std::string& busName);

		/// @brief Plays a sound
		/// @param sound The sound to be played
		/// @param busName The bus that the sound should be played to. If none is provided,
		/// it will default to Master
		/// @return A shared pointer to the voice that has been created to play the sound
		std::shared_ptr<Voice> play(const Sound& sound, const std::string& busName="Master");
	};
}