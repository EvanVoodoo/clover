#include "core/engine.hpp"
#include <chrono>
#include <core/scene.hpp>

namespace clvr {
	bool IsMouseMoving() {
		return Engine.GetInput()->IsMouseMoving();
	}

	bool IsKeyDown(unsigned int keycode) {
		return Engine.GetInput()->IsKeyDown(keycode);
	}

	bool IsMouseButtonDown(unsigned int button) {
		return Engine.GetInput()->IsMouseButtonDown(button);
	}

	bool IsMouseButtonUp(unsigned int button) {
		return !Engine.GetInput()->IsMouseButtonDown(button);
	}

	bool WasKeyJustReleased(unsigned int key) {
		return Engine.GetInput()->WasKeyJustReleased(key);
	}

	bool WasKeyJustPressed(unsigned int key) {
		return Engine.GetInput()->WasKeyJustPressed(key);
	}

	bool WasMouseButtonJustReleased(unsigned int button) {
		return Engine.GetInput()->WasMouseButtonJustReleased(button);
	}

	bool WasMouseButtonJustPressed(unsigned int button) {
		return Engine.GetInput()->WasMouseButtonJustPressed(button);
	}

	void GetMousePosition(int& x, int& y) {
		const int* pos = Engine.GetInput()->GetMousePosition();
		x = pos[0];
		y = pos[1];
	}
}

using namespace clvr;

EngineClass clvr::Engine;

bool EngineClass::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    m_ecs = new EntityComponentSystem();

	m_input = new Input();
	if (!m_input->Initialize())
		return false;

    m_window = new Window();
    if (!m_window->Initialize(hInstance, nCmdShow, SCREEN_WIDTH, SCREEN_HEIGHT, m_input))
        return false;

#ifdef CLOVER_EDITOR
    m_imgui = new ImGuiLayer();
#endif

    GetECS()->CreateSystem<SceneManager>();

    return true;
}

void EngineClass::Shutdown()
{
    if (m_imgui)
    {
		m_imgui->Shutdown();
		delete m_imgui;
		m_imgui = nullptr;
    }

    if (m_window)
    {
        m_window->Shutdown();
        delete m_window;
        m_window = nullptr;
    }

	if (m_input)
	{
        m_input->Shutdown();
		delete m_input;
        m_input = nullptr;
	}

    if (m_ecs) {
		delete m_ecs;
		m_ecs = nullptr;
    }
}

void EngineClass::Run()
{
    running = true;

    MSG msg;
    bool result;

    auto lastTime = std::chrono::steady_clock::now();

    while (running)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // If windows signals to end the application then exit out.
        if (msg.message == WM_QUIT)
        {
            running = false;
        }
        else
        {
			// calculate delta time
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;

#ifdef CLOVER_EDITOR
			m_imgui->BeginFrame();

            ImGui::DockSpaceOverViewport();

			GetECS()->InspectSystems(deltaTime);
#endif // CLOVER_EDITOR

			GetECS()->UpdateSystems(deltaTime);

            result = Frame(deltaTime);
            if (!result)
            {
                running = false;
            }
        }
    }
}

bool EngineClass::Frame(float dt)
{
    // Check if the user pressed escape and wants to exit the application.
    if (m_input->IsKeyDown(VK_ESCAPE))
    {
        return false;
    }

    return true;
}
