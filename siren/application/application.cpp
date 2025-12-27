#include "siren/siren.h"
#include "siren/Result.h"

#include "raylib.h"

#include <iostream>
#include <vector>
#include <span>
#include <thread>
#include <chrono>
#include <random>

// Testing
#include "siren/AudioContext.h"
#include "siren/Sound.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

int main() {
	std::cout << "SANDBOX" << std::endl << std::endl;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

	SetWindowState(FLAG_VSYNC_HINT);
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SANDBOX");
	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BLACK);
		DrawFPS(10, 10);

		EndDrawing();
	}

	return 0;
}