#include "siren/siren.h"
#include "siren/Result.h"

#include <iostream>
#include <vector>
#include <span>
#include <thread>

// Testing
#include "siren/AudioContext.h"
#include "siren/Sound.h"

using namespace siren;

int main() {
	std::cout << "SIREN SANDBOX" << std::endl << std::endl;

	AudioContext context;
	context.init();
	Sound sound = Sound::Stream("assets/white_ferrari.wav");
	if (!sound.isValid()) {
		std::cerr << "Invalid sound" << std::endl;
		return -1;
	}
	std::shared_ptr<Voice> voice = context.play(sound);
	//std::this_thread::sleep_for(std::chrono::milliseconds(100));
	//std::shared_ptr<Voice> voice2 = context.play(sound);

	// Test panning
	float increment = 0.002;
	while (voice->isPlaying()) {
		float currentPan = voice->getPan();
		if (currentPan < -0.9 || currentPan > 0.9) {
			increment = -increment;
		}
		voice->setPan(currentPan + increment);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	std::cin.get();

	context.deinit();

	return 0;
}