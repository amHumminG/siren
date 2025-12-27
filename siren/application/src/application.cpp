// Raylib / ImGui
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"
#include "imgui.h"

// Siren
#include "siren/AudioContext.h"

// Test Scenarios
#include "Scenario.h"

// Misc
#include <iostream>
#include <vector>	
#include <memory>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

int main() {
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SANDBOX");
	SetTargetFPS(60);
	rlImGuiSetup(true);

	siren::AudioContext context;
	context.init();

	bool mouseLock = false;
	Vector3 start = { 0.0f, 5.0f, 0.0f };
	Camera camera = { 0 };
	camera.position = start;
	camera.target = { start.x, start.y, 1.0f };
	camera.up = { 0.0f, 1.0f, 0.0f };
	camera.fovy = 45.0f;
	camera.projection = CAMERA_PERSPECTIVE;

	float moveSpeed = 2.0f;

	// Scenarios
	std::vector<std::unique_ptr<Scenario>> scenarios;
	
	// TODO: Add scenarios

	Scenario* selectedScenario = nullptr;

	while (!WindowShouldClose()) {
		float deltaTime = GetFrameTime();

		// Toggle mouse lock
		if (IsKeyPressed(KEY_C)) {
			mouseLock = !mouseLock;
			if (mouseLock) DisableCursor();
			if (!mouseLock) EnableCursor();
		}

		if (mouseLock && !ImGui::GetIO().WantCaptureMouse) {
			UpdateCamera(&camera, CAMERA_FIRST_PERSON);
		}

		Vector3 cameraFwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
		context.setListener(
			{ camera.position.x, camera.position.y, camera.position.z },
			{ cameraFwd.x, cameraFwd.y, cameraFwd.z },
			{ camera.up.x, camera.up.y, camera.up.z }
		);

		if (mouseLock) {

			// Reset to start pos
			if (IsKeyPressed(KEY_R)) {
				camera.position = start;
				camera.target = { start.x, start.y, 1.0f };
			}

			Vector3 movement = { 0.0f, 0.0f, 0.0f };

			if (IsKeyDown(KEY_SPACE)) {
				movement = Vector3Add(movement, { 0.0f, 1.0f, 0.0f });
			}
			if (IsKeyDown(KEY_LEFT_CONTROL)) {
				movement = Vector3Add(movement, { 0.0f, -1.0f, 0.0f });
			}

			Vector3 finalMovement = Vector3Scale(movement, moveSpeed * deltaTime);
			camera.position = Vector3Add(camera.position, finalMovement);
			camera.target = Vector3Add(camera.target, finalMovement);
		}

		// Scenario update
		if (selectedScenario) {
			selectedScenario->update(deltaTime, context);
		}

		// DRAW - Sceario
		BeginDrawing();
		ClearBackground(Color(28, 28, 28, 255));

		BeginMode3D(camera);
		DrawGrid(100, 1.0f);
		if (selectedScenario) {
			selectedScenario->draw3D();
		}
		EndMode3D();

		// DRAW - UI
		rlImGuiBegin();
		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
		style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
		style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);

		ImGui::Begin("Test Suite");
		ImGui::Text("Select Scenario:");
		ImGui::Separator();

		for (auto& scenario : scenarios) {
			bool isSelected = (selectedScenario == scenario.get());
			if (ImGui::Selectable(scenario->getName().c_str(), isSelected)) {
				if (selectedScenario) {
					selectedScenario->onStop(context);
				}
				selectedScenario = scenario.get();
				selectedScenario->onStart(context);
			}
		}
		ImGui::End();

		if (selectedScenario) {
			ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH - 260, 10), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_FirstUseEver);

			ImGui::Begin("Controls");
			selectedScenario->drawUI();
			ImGui::End();
		}

		rlImGuiEnd();
		EndDrawing();
	}

	// Cleanup
	if (selectedScenario) {
		selectedScenario->onStop(context);
	}
	context.deinit();
	rlImGuiShutdown();
	CloseWindow();

	return 0;
}