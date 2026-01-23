#pragma once
#include "siren/Decoder.h"
#include "siren/Resampler.h"
#include "siren/SirenMath.h"
#include <atomic>
#include <mutex>

enum class VoiceState {
	Inactive,
	Playing,
	Paused,
	Dead
};

enum class VoiceMode {
	Global, // Manual pan
	Spatial // Automatic pan and volume based on position and listener
};

namespace siren {

	class AudioBus;

	class Voice {
	private:
		std::atomic<AudioBus*> m_bus{ nullptr };

		std::unique_ptr<Decoder> m_decoder;
		Resampler m_resampler;
		
		std::atomic<VoiceState> m_state{ VoiceState::Inactive };
		std::atomic<bool> m_isMixing{ false }; // Gatekeeper

		VoiceMode m_mode = VoiceMode::Global;
		float m_volume = 1.0f;	// clamped between 0.0f and 1.0f
		float m_pan = 0.0f;		// clamped between -1.0f and 1.0f
		std::atomic<bool> m_destroyOnFinish{ false };
		std::atomic<float> m_isLooping{ false };
		std::atomic<float> m_pitch{ 1.0f };
		std::atomic<float> m_dopplerPitch{ 1.0f };
		float m_dopplerFactor = 1.0f;
		bool m_dopplerEffect = true;

		std::atomic<float> m_targetGainL{ 1.0f };
		std::atomic<float> m_targetGainR{ 1.0f };
		float m_currentGainL = 1.0f;
		float m_currentGainR = 1.0f;

		std::atomic<int64_t> m_seekFrame{ -1 }; // Seek request flag (-1 = No pending seek)
		uint32_t m_sampleRate = 0; // Stored to be used for frame to seconds conversion

		std::string m_tag; // Defaults to the tag that the sound held when this voice was constructed

		// Emitter data
		Vector3 m_position; // The position of the voice
		Vector3 m_previousPosition; // The position of the voice from the previous frame
		bool m_firstUpdate = true; // True if no update has been called on this voice yet

		float m_minDistance = 1.0f; // Minimum distance the voice can be heard from (for volume scaling)
		float m_maxDistance = 50.0f; // Maximum distance the voice can be heard from (for volume scaling)

		Vector3 m_velocity; // The velocity of the voice (will be used if provided for that frame)
		bool m_velocitySetThisFrame = false; // True if velocity has been manualy set for that frame
		float m_velocitySmoothing = 10.0f;

		void snapToTargetGain() {
			m_currentGainL = m_targetGainL.load();
			m_targetGainR = m_targetGainR.load();
		}

	public:
		Voice() = default;
		~Voice() = default;

		/// @brief Assigns a decoder for the voice to use
		/// @param decoder The decoder of an audio source
		void attachDecoder(std::unique_ptr<Decoder> decoder);

		/// @brief Initializes the voice state and prepares the DSP pipeline for play
		/// 
		/// Can be used as a hard reset for the Voice object and ensures a clean state before playback
		/// @return true if voice was sucessfully prepared, otherwise false
		bool prepare();

		/// @brief Mixes the buffer of its audio bus with decoded audio data if
		/// state is Playing
		/// 
		/// Supports [Mono, Stereo]
		/// @return True if voice is still alive, otherwise false
		[[nodiscard]] bool mix() noexcept;

		/// @brief Called every frame by the context. Updates all relevant audio logic for that frame
		/// @param deltaTime Frame time difference
		/// @param listener All necessary information about the listener
		/// @param globalDopplerScale The global doppler scale used to adjust doppler effect pitch impact
		void update(float deltaTime, const ListenerData& listener, float globalDopplerScale);

		/// @brief Sets voice state to Playing
		///
		/// If voice state is Inactive, it will play from the beginning
		void play();

		/// @brief Sets voice state to playing
		///
		/// Always plays sound from the beginning
		void playOneShot();

		/// @brief Sets voice state to Paused
		void pause();

		/// @brief Sets voice state to Inactive
		void stop();

		/// @brief Sets voice state to dead (Which will remove it from the context)
		void destroy();

		/// @param bus The bus that the voice will write to
		void setBus(AudioBus* bus);

		/// @param value New pan (clamped between -1.0f and 1.0f)
		void setPan(float value);

		/// @brief Sets voice volume (clamped between 0.0f and 1.0f)
		void setVolume(float value);

		/// @brief Sets voice pitch (clampled between 0.1f and 4.0f)
		void setPitch(float value);

		/// @brief Enables or disables the doppler effect for this voice (enabled by default)
		void setDopplerEffect(bool value);

		/// @brief Sets the doppler factor (1.0f by default). Use this to increase doppler effect pitch impact
		void setDopplerFactor(float value);

		/// @param value New value
		void setLooping(bool value);

		/// @param tag The tag given to the voice
		void setTag(const std::string& tag);

		/// @brief Never call this from the audio thread
		/// @return The tag given to the voice
		const std::string& getTag();

		/// @brief Sends a request to seek to a given time point
		/// @param timePoint Represents the position (in seconds) to jump to
		void seek(float timePoint);

		VoiceState getState();

		/// @return Current pan
		float getPan() const;

		/// @return The current volume of the voice (between 0.0f and 1.0f)
		float getVolume() const;

		/// @return The voice pitch (between 0.1f and 4.0f)
		float getPitch() const;

		/// @return The doppler factor used to manipulate the pitch impact of the doppler effect
		float getDopplerFactor() const;

		/// @return True if voice is looping, otherwise false
		bool isLooping() const;

		/// @return True if voice state = Playing, otherwise false
		bool isPlaying() const;

		/// @brief Sets the voice position and sets mode to spatial if
		/// voice is in any other mode
		void setPosition(const Vector3& pos);

		/// @brief Sets the voice velocity for a specific frame (optional manual override) 
		void setVelocity(const Vector3& vel);

		/// @brief Sets the velocity smoothing used by the exponential smoothing algorithm
		/// for smoothing approximated voice velocity
		void setVelocitySmoothing(float value);

		/// @return The current velocity of the voice
		///
		/// Note that if the velocity has not been manually provided this frame,
		/// it is approximated by the engine
		Vector3 getVelocity() const;

		/// @brief Sets voice mode to Global
		void setGlobal();

		/// @param minDistance The radius of full volume around the voice position
		/// @param maxDistance The maximum distance from the voice position that the 
		/// voice can be heard from
		void setDistance(float minDistance, float maxDistance);

		/// @return The radius of full volume around the voice position
		float getMinDistance();

		/// @return The maximum distance from the voice position that the 
		/// voice can be heard from
		float getMaxDistance();
	};
}