# Architecture Documentation

## Architecture Diagram
![Siren Architecture](diagrams/Architecture%20Diagram.svg)

## The Core Classes
#### **`DataSource`:** Abstract base class for reading bytes (I/O).
* **`FileDataSource`:** Reads from disk (`std::ifstream`).
* **`MemoryDataSource`:** Reads from a RAM buffer (`std::span` or `std::vector<std::byte>`).

<br>

#### **`Decoder`:** Abstract base class for parsing a `DataSource`.
* **`DecoderFactory`:** Static helper. Takes a `DataSource` and decides which `Decoder` to create for it.
* **`WavDecoder`:** Parses RIFF/WAV headers and converts integers to floats.

<br>

#### **`Sound`:** Represents an audio asset.
* **Variant 1 (Stream):** Holds the `std::string` file path.
* **Variant 2 (MemoryInternal):** Holds a `std::vector<std::byte>` (the raw sound data).
* **Variant 3 (MemoryExternal):** Holds a `std::span<std::byte>` (a view of the raw sound data).
* **Responsibility:** Keeps track of raw audio data.

<br>

#### **`Resampler`:** Used to pitch-shift raw audio data.

<br>

#### **`Voice`:** Represents a playback instance.
* **Ownership:** Holds and owns a `Decoder` and a `Resampler`.
* **States:** Inactive, Playing, Paused, Destroyed.
* **Method `update()`:** (Main thread) Performs all logic calcualtions (should be called every frame) 
* **Method `mix()`:** (Audio thread) Applies gain/pan/pitch etc, and mixes output into its designated `AudioBus`.

<br>

#### Bus Layer **`AudioBus`:** Represents a playing instance.
* **Responsibility:** Groups voices together for processing.

<br>

#### Context Layer* **`AudioContext`:** The main context class.
* **Voice Pool:** Owns a vector of `Voice` objects.
* **Bus Pool:** Owns a vector of `AudioBus` objects.
* **Listener:** Stores the position, orientation, and velocity of the "ears".

<br>

## The Data Flow. This is how data moves through the system.

#### Scenario: Loading & Playing a Short Effect (Gunshot). 
1. **User:** `Sound::MemoryInternal("asstes/sfx/bang.wav")`.
2. **`Sound`:** Reads raw audio data into memory.
3. **User:** `Bus = AudioContext.getBus(Name)`.
4. **User:** `Voice = AudioContext.CreateVoice(Sound, Bus)`.
5. **`Sound`: Creates a `DataSource` for the raw audio data.
6. **`AudioContext`:** Creates a decoder for for the `DataSource` using `DecoderFactory` and attaches it to `Voice` object.
7. **`AudioContext`:** Applies global settings to `Voice` object and routes it to the `AudioBus`.
8. **User:** `Voice->playOneShot()`
9. **`Voice`:** State changes from `Inactive` to `Playing`.
10. **`AudioContext`:** Mixes the `Voice` object each audio frame.

<br>

### The Execution Model. Two parallel threads.

**1. The Main Thread**

* **Frequency:** Variable (60 FPS / 144 FPS).
* **Responsibility:** Logic.
* User calls `play()`, `stop()`, `setVolume()`.
* User calls `setListenerPosition()`.
* `AudioSystem` updates state flags (e.g., sets `m_isPlaying = true`).

**2. The Audio Thread (High Priority Background)**

* **Frequency:** Fixed (Every ~10ms).
* **Responsibility:** Math.
* The OS requests a buffer (e.g., 512 frames).
* `AudioSystem` iterates over all **Active Voices**.
* `Voice::mix()` runs the math: `(Sample * Volume * 3D_Distance_Factor)`.
* The summed result is sent to the speakers.
