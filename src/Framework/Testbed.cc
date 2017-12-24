#include "Core/Main.h"
#include "Gfx/Gfx.h"
#include "Input/Input.h"
#include "Core/Time/Clock.h"
#include "IMUI/IMUI.h"
#include "imgui.h"

#include "Test.h"
#include "DebugDraw.h"

using namespace Oryol;

class Testbed : public App {
public:
	AppState::Code OnInit();
	AppState::Code OnRunning();
	AppState::Code OnCleanup();

private:
	void Simulate();
	void Interface();
	void Restart();

	bool showMenu = true;
	int32 testIndex = 0;
	int32 testSelection = 0;
	int32 testCount = 0;
	TestEntry* entry;
	Test* test;
	Settings settings;

	TimePoint lastTimePoint;
};
OryolMain(Testbed);

AppState::Code Testbed::OnInit() {
	Gfx::Setup(GfxSetup::Window(1024, 640, "Box2D Testbed"));
	Input::Setup();
	IMUI::Setup();
	CameraSetup cam;
	cam.Zoom = 10.0f;
	cam.WorldPosition.y = 20;
	g_camera.Setup(cam);
	g_debugDraw.Setup(Gfx::GfxSetup());

	testCount = 0;
	while (g_testEntries[testCount].createFcn != NULL)
	{
		++testCount;
	}

	testIndex = b2Clamp(testIndex, 0, testCount - 1);
	testSelection = testIndex;

	entry = g_testEntries + testIndex;
	test = entry->createFcn();
	Gfx::Subscribe([&](const GfxEvent & e) {
		//Handle resize events
		if (e.Type == GfxEvent::DisplayModified) {
			auto width = e.DisplayAttrs.FramebufferWidth;
			auto height = e.DisplayAttrs.FramebufferHeight;
			g_camera.ResizeViewport(width, height);
		}
	});
	Input::SubscribeEvents([&](const InputEvent & e) {
		//Handle input events
		switch (e.Type) {
		case InputEvent::KeyDown:
			if (!ImGui::GetIO().WantCaptureKeyboard) {
				switch (e.KeyCode) {
				case Key::Escape:
					requestQuit();
					break;
				case Key::Left:
					if (Input::KeyPressed(Key::LeftControl)) {
						b2Vec2 newOrigin(2.0f, 0.0f);
						test->ShiftOrigin(newOrigin);
					}
					else
						g_camera.WorldPosition.x -= 0.5f;
					break;
				case Key::Right:
					if (Input::KeyPressed(Key::LeftControl)) {
						b2Vec2 newOrigin(-2.0f, 0.0f);
						test->ShiftOrigin(newOrigin);
					}
					else
						g_camera.WorldPosition.x += 0.5f;
					break;
				case Key::Up:
					if (Input::KeyPressed(Key::LeftControl)) {
						b2Vec2 newOrigin(0.0f, -2.0f);
						test->ShiftOrigin(newOrigin);
					}
					else
						g_camera.WorldPosition.y += 0.5f;
					break;
				case Key::Down:
					if (Input::KeyPressed(Key::LeftControl)) {
						b2Vec2 newOrigin(0.0f, 2.0f);
						test->ShiftOrigin(newOrigin);
					}
					else
						g_camera.WorldPosition.y -= 0.5f;
					break;
				case Key::Home:
					g_camera.Zoom = 10.0f;
					g_camera.WorldPosition = { 0.0f, 20.0f };
					break;
				case Key::Z:
					// Zoom out
					g_camera.Zoom = b2Min(1.1f * g_camera.Zoom, 100.0f);
					break;

				case Key::X:
					// Zoom in
					g_camera.Zoom = b2Max(0.9f * g_camera.Zoom, 2.0f);
					break;
				case Key::R:
					Restart();
					break;
				case Key::Space:
					if (test) test->LaunchBomb();
					break;
				case Key::O:
					settings.singleStep = true;
					break;
				case Key::P:
					settings.pause = !settings.pause;
					break;
				case Key::Tab:
					showMenu = !showMenu;
					break;
				case Key::RightBracket:
					// Switch to next test
					++testSelection;
					if (testSelection == testCount)
					{
						testSelection = 0;
					}
					break;
				case Key::LeftBracket:
					// Switch to previous test
					--testSelection;
					if (testSelection < 0)
					{
						testSelection = testCount - 1;
					}
					break;
				default:
					if(test) test->Keyboard(e.KeyCode);
					break;
				}	
			}
			break;
		case InputEvent::KeyUp:
			if (!ImGui::GetIO().WantCaptureKeyboard)
				test->KeyboardUp(e.KeyCode);
			break;
		case InputEvent::MouseButtonDown:
		{
			auto pos = Input::MousePosition();
			auto pw = g_camera.ConvertScreenToWorld({ pos.x,g_camera.GetHeight() - pos.y });
			if (Input::KeyPressed(Key::LeftShift)) {
				test->ShiftMouseDown({ pw.x,pw.y });
			}
			else {
				test->MouseDown({ pw.x,pw.y });
			}
			break;
		}
		case InputEvent::MouseButtonUp:
			if (e.Button == MouseButton::Left && test) {
				auto pos = Input::MousePosition();
				auto pw = g_camera.ConvertScreenToWorld({ pos.x,g_camera.GetHeight() - pos.y });
				test->MouseUp({ pw.x,pw.y });
			}
			break;
		case InputEvent::MouseScrolling:
			if (!ImGui::GetIO().WantCaptureMouse) {
				if (e.Scrolling.y > 0) {
					g_camera.Zoom /= 1.1f;
				}
				else {
					g_camera.Zoom *= 1.1f;
				}
			}
			break;
		case InputEvent::MouseMove:
		{
			auto ps = Input::MousePosition();
			auto pw = g_camera.ConvertScreenToWorld({ ps.x, g_camera.GetHeight()-ps.y });
			if (test) test->MouseMove({ pw.x,pw.y });

			if (Input::MouseButtonPressed(MouseButton::Right)) {
				auto movement = e.Movement;
				auto diff = g_camera.ConvertScreenToWorldDir({ movement.x,-movement.y });
				g_camera.WorldPosition.x -= diff.x;
				g_camera.WorldPosition.y -= diff.y;
			}
			break;
		}
		}
	});
	this->lastTimePoint = Clock::Now();
	return App::OnInit();
}

AppState::Code Testbed::OnRunning() {
	
	Gfx::BeginPass();
	IMUI::NewFrame(Clock::LapTime(this->lastTimePoint));
	//Render Simulation
	Simulate();
	//Handle the UI.
	Interface();
	//Render the UI
	ImGui::Render();

	Gfx::EndPass();
	Gfx::CommitFrame();

	return Gfx::QuitRequested() ? AppState::Cleanup : AppState::Running;
}

AppState::Code Testbed::OnCleanup() {
	delete test;
	g_debugDraw.Discard();
	IMUI::Discard();
	Input::Discard();
	Gfx::Discard();
	return App::OnCleanup();
}

void Testbed::Simulate() {
	/*
	if (Input::KeyPressed(Key::Left)) {
	g_camera.WorldPosition.x -= 0.5f;
	}
	if (Input::KeyPressed(Key::Right)) {
	g_camera.WorldPosition.x += 0.5f;
	}
	if (Input::KeyPressed(Key::Up)) {
	g_camera.WorldPosition.y += 0.5f;
	}
	if (Input::KeyPressed(Key::Down)) {
	g_camera.WorldPosition.y -= 0.5f;
	}
	if (Input::KeyDown(Key::Tab)) {
	showMenu = !showMenu;
	}
	if (Input::KeyDown(Key::P)) {
	settings.pause = !settings.pause;
	}
	if (Input::KeyDown(Key::Space)) {
	test->LaunchBomb();
	}
	auto scrollDelta = Input::MouseScroll();
	if (scrollDelta.y > 0) {
	g_camera.Zoom *= 1.1;
	}
	else if (scrollDelta.y < 0) {
	g_camera.Zoom /= 1.1;
	}
	*/
	
	test->Step(&settings);

	test->DrawTitle(entry->name);

	if (testSelection != testIndex)
	{
		testIndex = testSelection;
		delete test;
		entry = g_testEntries + testIndex;
		test = entry->createFcn();
		g_camera.Zoom = 10.0f;
		g_camera.WorldPosition = { 0.0f, 20.0f };
		g_camera.Update();
	}
}

static bool sTestEntriesGetName(void*, int idx, const char** out_name)
{
	*out_name = g_testEntries[idx].name;
	return true;
}

void Testbed::Interface() {
	int menuWidth = 200;
	if (showMenu)
	{
		ImGui::SetNextWindowPos(ImVec2((float)g_camera.GetWidth() - menuWidth - 10, 10));
		ImGui::SetNextWindowSize(ImVec2((float)menuWidth, (float)g_camera.GetHeight() - 20));
		ImGui::Begin("Testbed Controls", &showMenu, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::PushAllowKeyboardFocus(false); // Disable TAB

		ImGui::PushItemWidth(-1.0f);

		ImGui::Text("Test");
		
		if (ImGui::Combo("##Test", &testIndex, sTestEntriesGetName, NULL, testCount, testCount))
		{
			delete test;
			entry = g_testEntries + testIndex;
			test = entry->createFcn();
			testSelection = testIndex;
		}

		ImGui::Separator();

		ImGui::Text("Vel Iters");
		ImGui::SliderInt("##Vel Iters", &settings.velocityIterations, 0, 50);
		ImGui::Text("Pos Iters");
		ImGui::SliderInt("##Pos Iters", &settings.positionIterations, 0, 50);
		ImGui::Text("Hertz");
		ImGui::SliderFloat("##Hertz", &settings.hz, 5.0f, 120.0f, "%.0f hz");
		ImGui::PopItemWidth();

		ImGui::Checkbox("Sleep", &settings.enableSleep);
		ImGui::Checkbox("Warm Starting", &settings.enableWarmStarting);
		ImGui::Checkbox("Time of Impact", &settings.enableContinuous);
		ImGui::Checkbox("Sub-Stepping", &settings.enableSubStepping);

		ImGui::Separator();

		ImGui::Checkbox("Shapes", &settings.drawShapes);
		ImGui::Checkbox("Joints", &settings.drawJoints);
		ImGui::Checkbox("AABBs", &settings.drawAABBs);
		ImGui::Checkbox("Contact Points", &settings.drawContactPoints);
		ImGui::Checkbox("Contact Normals", &settings.drawContactNormals);
		ImGui::Checkbox("Contact Impulses", &settings.drawContactImpulse);
		ImGui::Checkbox("Friction Impulses", &settings.drawFrictionImpulse);
		ImGui::Checkbox("Center of Masses", &settings.drawCOMs);
		ImGui::Checkbox("Statistics", &settings.drawStats);
		ImGui::Checkbox("Profile", &settings.drawProfile);

		ImVec2 button_sz = ImVec2(-1, 0);
		if (ImGui::Button("Pause (P)", button_sz))
			settings.pause = !settings.pause;

		if (ImGui::Button("Single Step (O)", button_sz))
			settings.singleStep = !settings.singleStep;

		if (ImGui::Button("Restart (R)", button_sz))
			Restart();

		if (ImGui::Button("Quit", button_sz))
			requestQuit();

		ImGui::PopAllowKeyboardFocus();
		ImGui::End();
	}
}
void Testbed::Restart() {
	delete test;
	entry = g_testEntries + testIndex;
	test = entry->createFcn();
}