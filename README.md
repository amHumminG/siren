# Siren Audio Engine
**Note:** No longer under development

Very simple audio engine designed for use in game engines. It uses Miniaudio for driver communication. The architecture of the engine has many flaws.
Especially with how it handles streaming and the lifecycle of voices and buses.

**Siren Supports**
* Simultaneous loading and playing 8 and 16-bit WAV-files.
* Full 3D spatialization including dynamic panning and attenuation models (linear, inverse distance and exponential).
* Seeking in playing sounds.
* Pitch shifting and real-time resampling.
* Doppler effect and emitter velocity approximation.
* Real time adjustments of volume, pan, pitch, dopplerfactor, min/max distance, attenuation models, and more.
* Buses and voice routing.
* Per-bus volume.
* Real time removal and addition of buses.

### Testbed
The repository also contains a testbed with a couple of scenarios used to play around with the engine's features.
It makes use of Raylib and ImGui for all rendering.

### Building
This project uses Visual Studio.
1. Open the .slnx (Solution) file in Visual Studio.
2. Set the build configuration.
3. Set the "application" project as startup project.
4. Build and run.
