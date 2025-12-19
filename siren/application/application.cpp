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

	AudioContext context;
	context.init();
	Sound whiteFerrari = Sound::Stream("assets/white_ferrari.wav", "White Ferrari - ANDREWBATES");
	Sound metalPipe = Sound::Internal("assets/metal_pipe.wav", "Metal Pipe Falling");

	std::shared_ptr<Voice> voice1 = context.play(whiteFerrari);
	//std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//std::shared_ptr<Voice> voice2 = context.play(sound);

	// Test panning
	float increment = 0.002;

	using Clock = std::chrono::steady_clock;
	auto lastTime = Clock::now();
	float pipeTimer = 0.0f;
	float seekTimer = 0.0f;
	
	std::chrono::milliseconds deltaTime(0);
	while (true) {
		// deltaTime
		auto currentTime = Clock::now();
		std::chrono::duration<float> fs = currentTime - lastTime;
		float deltaTime = fs.count();
		lastTime = currentTime;

		pipeTimer += deltaTime;
		if (pipeTimer >= 5.0f) {
			std::shared_ptr<Voice> voice = context.play(metalPipe);
			voice->setPan(dist(gen));

			pipeTimer -= 5.0f;
		}

		seekTimer += deltaTime;
		if (seekTimer >= 3.0f) {
			voice1->seek(44400.0f);

			seekTimer -= 3.0f;
		}

		if (voice1->isPlaying()) {
			float currentPan = voice1->getPan();
			if (currentPan < -0.9 || currentPan > 0.9) {
				increment = -increment;
			}
			voice1->setPan(currentPan + increment);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	context.deinit();

	return 0;
}