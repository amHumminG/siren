#pragma once
#include "siren/AudioBus.h"
#include "siren/Decoder.h"
#include "siren/Resampler.h"
#include "siren/SirenMath.h"
#include <atomic>
#include <memory>
#include <string>

namespace siren {

	class AudioBus;

	class Voice {
	public:
		enum class VoiceState {
			Inactive,
			Playing,
			Paused,
			Destroyed
		};

		enum class VoiceMode {
			Global, // Manual pan
			Spatial // Automatic pan and volume based on position and listener
		};

		enum class AttenuationModel {
			None,
			Linear,
			Inverse,
			Exponential
		};

		Voice() = default;
		~Voice() = default;

		/// @brief Starts Voice playback.
		///
		/// If the Voice is currently Paused or Inactive, it resumes or restarts playback.
		/// This method automatically flags the Voice as Reusable, meaning it will remain
		/// in memory (Inactive) after playback has finished, allowing it to be played again.
		/// @note If the Voice is already playing, or if the Voice has been destroyed, this function
		/// does nothing.
		/// @note Logs an error if called on a destroyed Voice
		void play();
		
		/// @brief Starts voice playback.
		///
		/// This function forces the voice into a "fire-and-forget" mode:
		/// 
		/// - Reusable is set to @c false
		/// 
		/// - Looping is disabled
		/// 
		/// - Playback resets to beginning of Sound
		/// 
		/// Once playback finishes, the voice will automatically be destroyed and removed from
		/// the AudioContext
		void playOneShot();

		/// @return @c true if the Voice is currently playing audio.
		bool isPlaying() const;

		/// @brief Pauses playback at the current position.
		/// 
		/// The Voice remains active in the AudioContext but is not mixed into the output.
		/// 
		/// The Voice can be resumed from this state at any point using play()
		void pause();

		/// @return @c true if the voice is currently paused.
		bool isPaused() const;

		/// @brief Stops playback and resets position to Sound start.
		///
		/// The behaviour depends on the Voice's reusability flag:
		/// 
		/// - For Reusable voices, position resets to the start of the Sound and the voice
		/// becomes inactive. The voice remains in the AudioContext and can be played again.
		/// 
		/// - For voices that are not Reusable, the Voice is immediately destroyed and removed
		/// from the AudioContext.
		void stop();

		/// @brief Permanently destroys the voice.
		/// 
		/// Marks the voice as Destroyed and blocks the calling thread briefly until the
		/// audio thread confirms it has finished accessing the memory.
		/// 
		/// @attention This function should always be called before de-allocation of any resources
		/// that the voice may rely on.
		/// 
		/// @warning The voice object should not be used after calling this.
		void destroy();

		/// @return @c true if the voice has finished playback or has been stopped.
		/// 
		/// @note This method will return @c true if the Voice is either Inactive or Destroyed.
		bool isFinished() const;

		/// @brief Controls whether the Voice persists after playback has finished.
		/// 
		/// If set to @c true, the Voice enters the Inactive state upon finishing playback and remains in memory.
		/// 
		/// If set to @c false, the Voice enters the Destroyed state upon finishing playback.
		void setReusable(bool value);

		/// @return @c true if the Voice persists in memory after playback finishes.
		bool isReusable() const;

		/// @brief Seeks to a specific time position in the Sound.
		/// @note The seek request is asynchronous and will be processed at the start of
		/// the next audio frame.
		/// @param timePoint The position in seconds to jump to.
		/// @attention There is currently no clamping functionality for this method.
		/// This means that seeking to a timePoint larger than the length of the Sound will result
		/// in no seek at all.
		void seek(float timePoint);

		/// @brief Disables 3D spatialization and switches to @b Global mode (default for all Voices).
		/// 
		/// In @b Global mode, the Voice has no position; volume and pan are controlled manually
		/// via setVolume() and setPan().
		/// 
		/// @note This also resets the pan to @c 0.0 (center).
		void setGlobal();

		/// @brief Routes the voice output to a specific AudioBus.
		/// 
		/// This method is safe to call during playback.
		/// The transition is instant; no cross-fading is applied between the old and new bus.
		/// @param bus The target bus to route output to.
		void setBus(std::shared_ptr<AudioBus> bus);

		/// @brief Sets whether the Voice should loop back to the start when reaching the end
		/// of the Sound
		/// @param value @c true to loop indefinitely, @c false to stop at the end.
		void setLooping(bool value);

		/// @return @c true if the Voice is set to loop indefinitely.
		bool isLooping() const;

		/// @brief Sets the output volume.
		/// 
		/// The value is clamped between @c 0.0 (silence) and @c 1.0 (full volume).
		/// @param value The volume.
		void setVolume(float value);

		/// @return The current volume (range: [0.0, 1.0]).
		float getVolume() const;

		/// @brief Sets the stereo pan position.
		/// 
		/// The value is clamped between @c -1.0 and @c 1.0.
		/// 
		/// - @c -1.0 : Hard left
		/// 
		/// - @c 0.0 : Center
		/// 
		/// - @c 1.0 : Hard right
		/// @param value The pan position.
		/// @note If Voice is in @b Spatial mode, this method does nothing.
		void setPan(float value);

		/// @return The current stereo pan setting (range: [-1.0, 1.0]).
		float getPan() const;

		/// @brief Sets the playback pitch/speed multiplier.
		/// 
		/// The value is clamped between @c 0.1 (10% speed) and @c 4.0 (400% speed).
		/// A value of 1.0 represents normal playback speed.
		/// @param value The pitch multiplier
		void setPitch(float value);

		/// @return The current pitch multiplier
		float getPitch() const;

		/// @brief Enables or disables 3D Doppler pitch shifting for this Voice.
		/// 
		/// If enabled, the pitch will automatically adjust based on the relative velocity between
		/// the Voice and the listener
		/// @param value @c true to enable, @c false to disable.
		void setDopplerEffect(bool value);

		/// @brief Sets the intensity of the Doppler Effect.
		/// 
		/// - @c 0.0 : No doppler effect (same as disabling it).
		/// 
		/// - @c 1.0 : Physically accurate doppler shift (Default).
		/// 
		/// - @c >1.0 : Exaggerated doppler shift.
		/// @param value The scalar factor for the doppler shift calculation.
		void setDopplerFactor(float value);

		/// @return The current Doppler Effect intensity factor.
		float getDopplerFactor() const;

		/// @brief Sets the 3D position of the voice in world space.
		/// 
		/// Calling this method automatically switches the voice mode to @b Spatial.
		/// 
		/// For voices in @b Spatial mode, the following is true:
		/// 
		/// - Final volume is calculated based on the voice volume and its position relative 
		/// to the listener position (Provided by the AudioContext).
		///
		/// - Pan is overriden completely and is calculated solely based on Voice and listener position.
		/// @param pos The position vector.
		void setPosition(const Vector3& pos);

		/// @brief Manually sets the Voice velocity for this frame.
		/// 
		/// If not called, the engine automatically approximates velocity based on position changes.
		/// Use this if your game physics engine already knows the exact velocity of the object holding the Voice.
		/// @param vel The velocity vector in units per second.
		void setVelocity(const Vector3& vel);

		/// @return The current velocity of the Voice (either manual or apporximated).
		Vector3 getVelocity() const;

		/// @brief Sets the smoothing factor for automatic velocity approximation (defaults to @c 10.0)
		/// 
		/// Used when velocity is not manually set.
		/// 
		/// Lower values e.g. @c 2.0 means more smoothing (can sound laggy).
		/// 
		/// Higher values e.g. @c 20.0 means less smoothing (can sound jittery).
		/// @param value The smoothing factor.
		void setVelocitySmoothing(float value);

		/// @brief Configures the distance attenuation model for 3D spatialization.
		/// 
		/// Within @p minDistance, the Voice is at full volume (the manually set volume).
		/// 
		/// As distance increases towards @p maxDistance, volume fades out.
		/// @param minDistance The radius of full volume around the voice position
		/// @param maxDistance The distance at which volume fades to silence.
		/// @note Input values are automatically clamped to valid ranges.
		void setDistance(float minDistance, float maxDistance);

		/// @return The minimum distance (full volume radius) for spatial attenuation.
		float getMinDistance();

		/// @return The maximun distance (silence radius) for spatial attenuation.
		float getMaxDistance();

		void setAttenuationModel(AttenuationModel model);

		void setRolloff(float value);

		float getRolloff();

		/// @brief Assigns a custom string tag for identification of this Voice.
		/// 
		/// Defaults to the tag of the Sound that the Voice was created with.
		/// 
		/// Potentially useful for debugging.
		/// @param tag The identifier string.
		void setTag(const std::string& tag);

		/// @brief Retrieves the custom tag assigned to this Voice.
		/// @return The tag string.
		const std::string& getTag();

	private:
		friend class AudioContext;

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
		void update(float deltaTime, const ListenerData& listener, float globalDopplerFactor);

		/// @return The current state of the Voice.
		VoiceState getState();

		std::unique_ptr<Decoder> m_decoder;
		Resampler m_resampler;
		std::atomic<std::shared_ptr<AudioBus>> m_bus{ nullptr };

		std::atomic<VoiceState> m_state{ VoiceState::Inactive };
		VoiceMode m_mode = VoiceMode::Global;
		std::atomic<bool> m_isMixing{ false }; // Gatekeeper
		std::string m_tag; // Defaults to the tag that the sound held when this voice was constructed
		std::atomic<bool> m_isReusable{ true };
		std::atomic<bool> m_isLooping{ false };

		float m_volume = 1.0f;	// clamped between 0.0f and 1.0f
		float m_pan = 0.0f;		// clamped between -1.0f and 1.0f
		std::atomic<float> m_pitch{ 1.0f };

		std::atomic<float> m_dopplerPitch{ 1.0f };
		float m_dopplerFactor = 1.0f;
		bool m_dopplerEffect = true;

		std::atomic<float> m_targetGainL{ 1.0f };
		std::atomic<float> m_targetGainR{ 1.0f };
		float m_currentGainL = 1.0f;
		float m_currentGainR = 1.0f;
		std::atomic<float> m_snapGainRequested{ false };

		std::atomic<int64_t> m_seekFrame{ -1 }; // Seek request flag (-1 = No pending seek)
		uint32_t m_sampleRate = 0; // Stored to be used for frame to seconds conversion

		// 3D Emitter data
		bool m_firstUpdate = true; // True if no update has been called on this voice yet
		Vector3 m_position; // The position of the voice
		Vector3 m_previousPosition; // The position of the voice from the previous frame

		Vector3 m_velocity; // The velocity of the voice (will be used if provided for that frame)
		bool m_velocitySetThisFrame = false; // True if velocity has been manualy set for that frame
		float m_velocitySmoothing = 10.0f;

		float m_minDistance = 1.0f; // Minimum distance the voice can be heard from (for volume scaling)
		float m_maxDistance = 50.0f; // Maximum distance the voice can be heard from (for volume scaling)

		AttenuationModel m_attenuationModel = AttenuationModel::Linear;
		float m_rolloff = 1.0f;
	};
}