#pragma once
#include "raylib.h"
#include <string>

namespace siren { 
	class AudioContext; 
}

class Scenario {
public:
	virtual ~Scenario() = default;

	virtual void onStart(siren::AudioContext& context) = 0;
	virtual void onStop(siren::AudioContext& context) = 0;

	virtual void update(float deltaTime, siren::AudioContext& context) = 0;
	
	virtual void draw3D() = 0; // Called inside BeginMode3D
	virtual void drawUI() = 0; // Called inside rlImGuiBegin

	virtual std::string getName() = 0;
};
