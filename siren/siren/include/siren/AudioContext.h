#pragma once
#include "siren/AudioBus.h"
#include "siren/Voice.h"
#include "siren/Sound.h"
#include "siren/SirenMath.h"
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

struct ma_device;

namespace siren {

	enum class CoordinateSystem {
		RightHanded,
		LeftHanded
	};

	class AudioContext {
	public:

		AudioContext();
		~AudioContext();

		/// @brief Initializes the AudioContext.
		/// @return @c true if initialization was successful.
		bool init();

		/// @brief Shuts down the AudioContext and releases its resources.
		/// 
		/// Stops the audio thread and uninitializes the backend.
		/// Safe to call even if the AudioContext is already uninitialized.
		/// @return @c true if shutdown was successful.
		bool deinit();

		/// @brief Updates the AudioContext state. Must be called once per frame.
		/// @param deltaTime The time elapsed since the last frame (in seconds).
		void update(float deltaTime);

		/// @brief Instantly destroys all Voices and invalidates pending commands.
		/// 
		/// @note The calling thread blocks until the audio thread confirms
		/// that the flush is complete.
		void flush();

		/// @brief Defines the 3D coordinate system used by the host application.
		/// 
		/// This setting determines how the engine calculates directional audio.
		/// @param system The coordinate system convention (default is LeftHanded).
		void setCoordinateSystem(CoordinateSystem system);

		/// @brief Updates the listener position and orientation.
		/// 
		/// @param pos World position vector.
		/// @param fwd Forward direction vector.
		/// @param up Up direction vector.
		void setListener(const Vector3& pos, const Vector3& fwd, const Vector3& up);

		/// @brief Updates the listener world position.
		/// @param pos World position vector.
		void setListenerPos(const Vector3& pos);

		/// @brief Updates the listener orientation.
		/// 
		/// The engine automatically normalizes the input vectors.
		/// 
		/// The listener's right-vector is calculated based on the coordinate system setting.
		/// @param fwd Forward direction vector.
		/// @param up  Up direction vector.
		void setListenerOrientation(const Vector3& fwd, const Vector3& up);

		/// @brief Manually sets the listener velocity for this frame.
		/// 
		/// Calling this function disables the automatic velocity approximation for one frame.
		/// 
		/// Call this method every frame if you have access to the velocity of the listener 
		/// through a physics engine.
		/// 
		/// @param vel Velocity vector in units per second.
		void setListenerVelocity(const Vector3& vel);

		/// @brief Sets the smoothing factor for the listener's automatic velocity approximation.
		/// 
		/// Only used if velocity is not manually provided every frame.
		/// 
		/// Lower values e.g. @c 2.0 means more smoothing (can sound laggy).
		/// 
		/// Higher values e.g. @c 20.0 means less smoothing (can sound jittery).
		/// @param value Smoothing factor. Defaults to @c 20.0.
		void setListenerVelocitySmoothing(float value);

		/// @brief Sets the default velocity smoothing factor for newly created Voices.
		/// 
		/// Lower values e.g. @c 2.0 means more smoothing (can sound laggy).
		/// 
		/// Higher values e.g. @c 20.0 means less smoothing (can sound jittery).
		/// @param value Smoothing factor. Defaults to @c 10.0
		void setDefaultVelocitySmoothing(float value);

		/// @brief Sets the default attenuation model for newly created Voices.
		/// 
		/// @param model Attenuation model. Defaults to Linear.
		void setDefaultAttenuationModel(Voice::AttenuationModel model);

		/// @brief Sets the global multiplier for all Doppler Effects.
		///
		/// - @c 0.0 : No Doppler Effect (same as disabling it).
		/// 
		/// - @c 1.0 : Physically accurate (Default).
		/// 
		/// - @c >1.0 : Exaggerated.
		/// @param value Global multiplier.
		void setGlobalDopplerFactor(float value);

		/// @return The global Doppler Effect multiplier.
		float getGlobalDopplerFactor() const;

		/// @brief Retrieves a copy of the current listener state.
		/// @return A ListenerData struct containing position, orientation, and velocity.
		ListenerData getListener() const;

		/// @brief Creates a new AudioBus with the specified name.
		/// 
		/// The new bus is automatically connected to the internal mixing.
		/// 
		/// @note The name "Master" is reserved and cannot be used.
		/// @param busName The unique identifier for the bus.
		/// @return A pointer to the new bus, or @c nullptr if a bus with that name
		/// already exists.
		std::shared_ptr<AudioBus> createBus(const std::string& busName);

		/// @brief Removes a user-defined bus from the AudioContext.
		/// 
		/// Any Voices routed to this bus will continue to play, but they will become
		/// inaudible unless they are rerouted.
		/// @note This function does nothing if called on "Master" or a non-existent bus.
		/// @param busName The name of the bus to remove.
		void removeBus(const std::string& busName);

		/// @brief Retrieves a bus by name.
		/// @param busName The name of the bus.
		/// @return The bus pointer, or @c nullptr if not found.
		std::shared_ptr<AudioBus> getBus(const std::string& busName) noexcept;

		/// @brief Creates a new Voice instance to play the specified Sound.
		/// 
		/// This is the primary method for playing audio. It creates a Voice object that allows
		/// control over playback properties.
		/// 
		/// The AudioContext will also hold a reference to the voice internally until the
		/// Voice has been destroyed.
		/// 
		/// @attention If the Voice handle returned by this method is lost, the Voice can no
		/// longer be modified. This means that a looping Voice will continue to play until
		/// the AudioContext has been flushed or deinitialized.
		/// 
		/// @param sound The Sound asset to use as a source. Must be valid.
		/// @param bus The Audiobus to route output audio to. Defaults to Master if @c nullptr.
		/// @return A shared pointer to the new Voice, or @c nullptr if creation failed.
		std::shared_ptr<Voice> createVoice(const Sound& sound, std::shared_ptr<AudioBus> bus = nullptr);

		/// @brief Convenience alternative for Voice instance creation.
		/// @param sound The Sound asset to use as a source. Must be valid.
		/// @param busName The name of the target AudioBus. Defaults to Master
		/// if no bus with name @p busName exists.
		/// @return A shared pointer to the new Voice, or @c nullptr if creation failed.
		std::shared_ptr<Voice> createVoice(const Sound& sound, const std::string& busName) ;

	private:
		/// @brief Writes audio data to the device
		/// @param pDevice The device
		/// @param pOutput Buffer for the audio data to be written to
		/// @param pInput Unused (used for audio recording)
		/// @param frameCount The number of frames requested by the audio device
		static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, uint32_t frameCount);

		/// @brief Updates the list of buses that the audio thread can access to match the
		/// main thread
		void refreshBusesAudio();

		bool m_initialized = false;
		std::unique_ptr<ma_device> m_device; // The device used for audio playback

		std::mutex m_busMutex;
		std::unordered_map<std::string, std::shared_ptr<AudioBus>> m_busesMain; // Contains all audio buses
		std::shared_ptr<AudioBus> m_masterBus = nullptr;
		using BusList = std::vector<std::shared_ptr<AudioBus>>;
		std::atomic<std::shared_ptr<BusList>> m_busesAudio; // Snapshot of the buses accessed by audio thread

		struct PendingVoiceNode {
			std::shared_ptr<Voice> voice;
			PendingVoiceNode* next = nullptr;
		};
		std::atomic<PendingVoiceNode*> m_inboxHead{ nullptr }; // Linked list of pending voices to be played

		std::vector<std::shared_ptr<Voice>> m_voicesMain;	// Only accessed by simulation thread
		std::vector<std::shared_ptr<Voice>> m_voicesAudio;	// Only accessed by audio thread

		std::atomic<bool> m_flushRequested{ false };
		std::atomic<bool> m_flushCompleted{ false };
		
		CoordinateSystem m_coordinateSystem = CoordinateSystem::LeftHanded;
		ListenerData m_listener; // Represents the listener (most likely the player)
		Vector3 m_previousListenerPos;
		bool m_listenerVelocitySetThisFrame = false;
		mutable std::mutex m_listenerMutex; // Mutable for use in getListener()
		
		float m_listenerVelocitySmoothing = 20.0f;
		float m_defaultVelocitySmoothing = 10.0f; // Default set to all new voices

		Voice::AttenuationModel m_defaultAttenuationModel = Voice::AttenuationModel::Linear;

		float m_globalDopplerFactor = 1.0f;
	};
}