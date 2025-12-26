#include "siren/siren.h"
#include "siren/Result.h"

#include <iostream>
#include <vector>
#include <span>
#include <thread>
#include <chrono>
#include <random>

// Testing
#include "siren/AudioContext.h"
#include "siren/Sound.h"

using namespace siren;

int main() {
	std::cout << "SIREN SANDBOX" << std::endl << std::endl;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

	// Context creation
	AudioContext context;
	context.init();
	context.setListener(
		Vector3(0.0f, 0.0f, 0.0f),
		Vector3(0.0f, 0.0f, 1.0f),
		Vector3(0.0f, 1.0f, 0.0f)
	);

	// Bus creation
	context.createBus("Music");
	context.createBus("SFX");
	context.setBusVolume("SFX", 0.3f);
	context.setBusVolume("Master", 0.7f);

	// Sound creation
	Sound whiteFerrari = Sound::Stream("assets/white_ferrari.wav", "White Ferrari - ANDREWBATES");
	Sound bloodRunWarm = Sound::Internal("assets/blood_run_warm.wav", "Blood Run Warm - South Arcade");
	Sound metalPipe = Sound::Internal("assets/metal_pipe.wav", "Metal Pipe Falling");

	// Sound playback
	std::shared_ptr<Voice> voice1 = context.play(whiteFerrari, "Music");
	//std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Delay
	//std::shared_ptr<Voice> voice2 = context.play(bloodRunWarm, "Master");
	voice1->setLooping(true);
	voice1->setDistance(0.1f, 50.0f);

	// Ping pong testing
	float angle = 0.0f;
	float frequency = 0.1f;

	// Timers
	float pipeTimer = 0.0f;

	using Clock = std::chrono::steady_clock;
	auto lastTime = Clock::now();
	std::chrono::milliseconds deltaTime(0);
	while (true) {
		// Delta Time
		auto currentTime = Clock::now();
		std::chrono::duration<float> fs = currentTime - lastTime;
		float deltaTime = fs.count();
		lastTime = currentTime;

		pipeTimer += deltaTime;
		if (pipeTimer >= 5.0f) {
			//std::shared_ptr<Voice> voice = context.play(metalPipe, "SFX");
			//voice->setPan(dist(gen));

			if (voice1) {
				if (voice1->isPlaying()) {

				}
				else {

				}
			}

			pipeTimer -= 5.0f;
		}

			// Angle rotating between 0 and 360 degrees
			angle += frequency * 6.28318f * deltaTime;

			if (voice1->isPlaying()) {
				// Rotating voice around origo with radius r
				float r = 30.0f;
				float x = cos(angle) * r;
				float y = 0.0f;
				float z = sin(angle) * r;
				std::cout << "X: " << round(x) << "  |  Z: " << round(z) << std::endl;
				voice1->setPosition(Vector3(x, y, z));
			}
		}

	context.deinit();

	return 0;
}