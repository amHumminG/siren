# Siren Audio Engine
**Note:** No longer under development

A very simple object-oriented C++ audio engine developed purely as a learning exercise. It uses Miniaudio for driver communication. The architecture of the engine has many flaws. Especially with how it handles asset streaming and the voice/bus life-cycles. Use at your own risk :)

**Siren Supports**
* Simultaneous loading and playing 8 and 16-bit WAV-files.
* Full 3D spatialization including dynamic panning and a few attenuation models (linear, inverse distance and exponential).
* Seeking in playing sounds.
* Pitch shifting and real-time resampling.
* Doppler effect and emitter velocity approximation.
* Real time adjustments of volume, pan, pitch, dopplerfactor, min/max distance, attenuation models, and more.
* Buses and voice routing.
* Per-bus volume.
* Real time removal and addition of buses.

### Testbed
The repository also contains a testbed/sandbox environment with a couple of scenarios used to play around with the engine's features. 
It uses Raylib and ImGui for all rendering.

### Building
This project uses Visual Studio.
1. Open the .slnx (solution) file in Visual Studio.
2. Set the build configuration.
3. Set the "application" project as startup project.
4. Build and run.
