# Architecture Documentation

## The Core Classes
#### The Data Layer (I/O)* **`DataSource` (Interface):** Abstract base class for reading bytes.
* **`FileDataSource`:** Reads from disk (`std::ifstream`).
* **`MemoryDataSource`:** Reads from a RAM buffer (`std::span`).
* **`Decoder` (Interface):** Abstract base class for parsing audio formats.
* **`WavDecoder`:** Parses RIFF/WAV headers and converts integers to floats.
* **`DecoderFactory`:** Static helper. Peeks at the first 4 bytes of a `DataSource` to decide which `Decoder` to create.

<br>

#### The Resource Layer (Sheet Music)* **`Sound`:** Represents the audio asset. It is read-only and shared.
* **Variant A (In-Memory):** Holds a `std::vector<float>` of the entire sound (for SFX).
* **Variant B (Stream):** Holds the `std::string` file path (for Music).
* **Metadata:** Sample rate, channel count, duration.
* *Note: Does NOT track playback position.*

<br>

#### The Playback Layer (Musician)* **`Voice`:** Represents a playing instance.
* **Reference:** Holds a `std::shared_ptr<Sound>`.
* **State:** Position (`cursor`), Volume, Pitch, Looping, 3D Position (`x,y,z`).
* **Decoder Ownership:**
* If playing a Stream: Owns a unique `std::unique_ptr<Decoder>`.
* If playing Memory: Reads directly from `Sound`'s buffer.

* **Method `mix(buffer)`:** The math function. Reads data, applies volume/3D panning, and adds to the output.

<br>

#### The Manager Layer (Compositor)* **`AudioSystem`:** The main context class.
* **Voice Pool:** Owns a fixed vector of `Voice` objects (e.g., 64).
* **Asset Cache:** Optional `std::map<string, Sound>` to prevent reloading.
* **Listener:** Stores the (X,Y,Z) and Orientation of the "ears" (Player Camera).
* **Method `play(Sound*)`:** Finds a free Voice, wires it to the Sound, and starts it.

<br>

## The Data Flow (The Lifecycle of a Sound)This is how data moves through the system in the two main scenarios.

#### Scenario A: Loading & Playing a Short Effect (Gunshot)1. **User:** `system.createSound("bang.wav")`
2. **System:** Creates `FileDataSource` -> `DecoderFactory`.
3. **Decoder:** Decodes **ALL** samples immediately into a `vector<float>`.
4. **System:** Wraps vector in a `Sound` object. Deletes Decoder.
5. **User:** `system.play(sound)`
6. **System:** Finds free `Voice`. Points `Voice` to `Sound`'s vector.
7. **Callback:** `Voice` copies floats from RAM to Speaker Buffer.

#### Scenario B: Streaming Music (Background Track)1. **User:** `system.createSound("music.wav", StreamMode::Stream)`
2. **System:** Reads header only. Wraps file path in a `Sound` object.
3. **User:** `system.play(sound)`
4. **System:** Finds free `Voice`.
5. **Voice:** Creates a **NEW** `FileDataSource` and `WavDecoder` for itself.
6. **Callback:** `Voice` asks `Decoder` to read 512 bytes from disk -> converts to float -> adds to Speaker Buffer.

<br>

### The Execution Model (Threading) Two parallel timelines.

**1. The Main Thread (Typically A Game Loop)**

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
