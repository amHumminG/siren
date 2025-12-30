// Raylib / ImGui
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"
#include "imgui.h"

// Siren
#include "siren/AudioContext.h"

// Test Scenarios
#include "Scenario.h"
#include "OrbitScenario.h"
#include "SoundscapeScenario.h"

// Misc
#include <iostream>
#include <vector>	
#include <memory>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

void SetupImGuiStyle(float alpha) {
	ImGuiStyle& style = ImGui::GetStyle();
	style.Alpha = alpha;

	// Colors
	ImVec4 inactiveColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	ImVec4 hoveredColor = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	ImVec4 activeColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	ImVec4 whiteColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	style.Colors[ImGuiCol_TitleBg] = inactiveColor;
	style.Colors[ImGuiCol_TitleBgActive] = activeColor;

	style.Colors[ImGuiCol_Button] = inactiveColor;
	style.Colors[ImGuiCol_ButtonHovered] = hoveredColor;
	style.Colors[ImGuiCol_ButtonActive] = activeColor;

	style.Colors[ImGuiCol_Header] = inactiveColor;
	style.Colors[ImGuiCol_HeaderHovered] = hoveredColor;
	style.Colors[ImGuiCol_HeaderActive] = activeColor;

	style.Colors[ImGuiCol_FrameBg] = inactiveColor;
	style.Colors[ImGuiCol_FrameBgHovered] = hoveredColor;
	style.Colors[ImGuiCol_FrameBgActive] = activeColor;
	style.Colors[ImGuiCol_CheckMark] = whiteColor; // White checkmark

	style.Colors[ImGuiCol_SliderGrab] = whiteColor;
	style.Colors[ImGuiCol_SliderGrabActive] = whiteColor;

	style.Colors[ImGuiCol_ResizeGrip] = inactiveColor;
	style.Colors[ImGuiCol_ResizeGripHovered] = hoveredColor;
	style.Colors[ImGuiCol_ResizeGripActive] = activeColor;

	style.Colors[ImGuiCol_SeparatorHovered] = hoveredColor;
	style.Colors[ImGuiCol_SeparatorActive] = activeColor;

	// Optional: Rounding for a smoother look
	style.WindowRounding = 4.0f;
	style.FrameRounding = 4.0f;
	style.GrabRounding = 4.0f;

}

int main() {
	InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Siren Testbed");
	Image icon = LoadImage("assets/images/icon.png");
	SetWindowIcon(icon);

	SetTargetFPS(60);
	rlImGuiSetup(true);

	siren::AudioContext context;
	context.init();
	context.setCoordinateSystem(CoordinateSystem::RightHanded);
	context.createBus("Music");
	context.createBus("SFX");
	float masterVol = context.getBusVolume("Master");
	float musicVol = context.getBusVolume("Music");
	float sfxVol = context.getBusVolume("SFX");

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
	
	// Add scenarios
	scenarios.push_back(std::make_unique<OrbitScenario>());
	scenarios.push_back(std::make_unique<SoundscapeScenario>());

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
		SetupImGuiStyle(0.7f);

		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_Once);

		ImGui::Begin("Menu");

		ImGui::SeparatorText("Mixer");

		// Define size for faders
		ImVec2 sliderSize(30, 100);

		// Master
		ImGui::BeginGroup();
		ImGui::Text("Master");
		if (ImGui::VSliderFloat("##Master", sliderSize, &masterVol, 0.0f, 1.0f, "")) {
			context.setBusVolume("Master", masterVol);
		}
		ImGui::EndGroup();

		ImGui::SameLine(); // Put next slider to the right

		// Music
		ImGui::BeginGroup();
		ImGui::Text("Music");
		if (ImGui::VSliderFloat("##Music", sliderSize, &musicVol, 0.0f, 1.0f, "")) {
			context.setBusVolume("Music", musicVol);
		}
		ImGui::EndGroup();

		ImGui::SameLine();

		// SFX
		ImGui::BeginGroup();
		ImGui::Text("SFX");
		if (ImGui::VSliderFloat("##SFX", sliderSize, &sfxVol, 0.0f, 1.0f, "")) {
			context.setBusVolume("SFX", sfxVol);
		}
		ImGui::EndGroup();

		// Display numeric values below
		ImGui::Text("%.2f  %.2f  %.2f", masterVol, musicVol, sfxVol);

		ImGui::Separator();

		ImGui::Text("Scenarios:");
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
			ImGui::SetNextWindowPos(ImVec2(WINDOW_WIDTH - 260, 10), ImGuiCond_Once);
			ImGui::SetNextWindowSize(ImVec2(250, 500), ImGuiCond_Once);

			ImGui::Begin("Scenario Controls");
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