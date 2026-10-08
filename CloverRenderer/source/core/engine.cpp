#include "core/engine.hpp"
#include <chrono>
#include <core/scene.hpp>
#include <rendering/renderer.hpp>

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

    const std::wstring ToWString(const std::string& str)
    {
        // Source - https://stackoverflow.com/a/246811
        // Posted by Matt Dillard, modified by community. See post 'Timeline' for change history
        // Retrieved 2026-09-21, License - CC BY-SA 3.0

        return std::wstring(str.begin(), str.end());
    }

    bool IsDevEnvironment() {
		// check if environment variable CLVR_DEV_MODE is set to "1"
        char* envVar = nullptr;
        size_t len = 0;
        errno_t err = _dupenv_s(&envVar, &len, "CLVR_DEV_MODE");

        bool isDev = (err == 0 && envVar != nullptr && std::string(envVar) == "1");

        free(envVar); // _dupenv_s allocates with malloc — you own this memory, must free it
        return isDev;
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

	m_resourceManager = new ResourceManager();
    if (!m_resourceManager)
        return false;

#ifdef CLOVER_EDITOR
    m_imgui = new ImGuiLayer();
#else
    SetEngineMode(EngineMode::Playing);
#endif
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

	if (m_resourceManager)
	{
		delete m_resourceManager;
		m_resourceManager = nullptr;
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
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
		if (!running) break;
        
		// calculate delta time
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        result = Frame(deltaTime);
        m_input->EndFrame();
        if (!result)
        {
            running = false;
        }
    }
}

void EngineClass::MainMenuBar()
{
    ImGui::DockSpaceOverViewport();
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            const bool canEditScene = GetEngineMode() == EngineMode::Editing;

            // signature: MenuItem(label, shortcut, selected, enabled)
            if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, canEditScene)) {
                GetECS()->GetSystem<SceneManager>().SaveSceneToFile(
                    GetResourceManager()->GetPath(ResourceManager::Directory::Assets, "example.json"));
            }
            if (ImGui::MenuItem("Load Scene", "Ctrl+L", false, canEditScene)) {
                GetECS()->GetSystem<SceneManager>().LoadSceneFromFile(
                    GetResourceManager()->GetPath(ResourceManager::Directory::Assets, "example.json"));
            }
            ImGui::EndMenu();
        }

        if (GetEngineMode() == EngineMode::Editing) {
            if (ImGui::BeginMenu("Play")) {
                if (ImGui::MenuItem("Play", "F5")) {
                    GetECS()->GetSystem<SceneManager>().Play();
                }
                ImGui::EndMenu();
            }
        }
        else if (GetEngineMode() == EngineMode::Playing) {
            if (ImGui::BeginMenu("Stop")) {
                if (ImGui::MenuItem("Stop", "F5")) {
                    GetECS()->GetSystem<SceneManager>().Stop();
                }
                ImGui::EndMenu();
            }
        }

        const float labelW = ImGui::CalcTextSize("X").x + ImGui::GetStyle().FramePadding.x;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - labelW - ImGui::GetStyle().WindowPadding.x);

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.85f, 0.15f, 0.15f, 1.0f)); // hover
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.65f, 0.10f, 0.10f, 1.0f)); // pressed
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.65f, 0.10f, 0.10f, 1.0f)); // while the menu is open

        const bool open = ImGui::BeginMenu("X");

        ImGui::PopStyleColor(3); // pop before the popup contents so "Quit" isn't red too

        if (open)
        {
            if (ImGui::MenuItem("Quit", "Alt+F4"))
                running = false;
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void EngineClass::SetEngineMode(EngineMode mode) { 
    m_editorMode = mode; 
	GetECS()->SetActiveRegistry(mode);
}

bool EngineClass::GameWindowFocused()
{
	return GetECS()->GetSystem<Renderer>().GameWindowFocused();
}

bool EngineClass::Frame(float dt)
{
#ifdef CLOVER_EDITOR
    if (WasKeyJustPressed(VK_F5)) {
        auto& sm = GetECS()->GetSystem<SceneManager>();
        (GetEngineMode() == EngineMode::Editing) ? sm.Play() : sm.Stop();
    }

    m_imgui->BeginFrame();

    MainMenuBar();

    GetECS()->InspectSystems(dt);
#endif // CLOVER_EDITOR
    GetECS()->UpdateSystems(dt);
    GetECS()->RemoveDeleted();
    GetECS()->RenderSystems();
    return true;
}
