#include "core/engine.hpp"
#include <chrono>
#include <core/scene.hpp>
#include <rendering/renderer.hpp>
#include <core/imgui_helpers.hpp>

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

static std::string NormalizeSceneName(std::string name)
{
    name.erase(0, name.find_first_not_of(" \t"));      // trim left
    name.erase(name.find_last_not_of(" \t") + 1);      // trim right

    constexpr std::string_view ext = ".json";          // user typed the extension: strip it
    if (name.size() >= ext.size()) {
        const std::string_view tail = std::string_view(name).substr(name.size() - ext.size());
        if (std::equal(tail.begin(), tail.end(), ext.begin(),
                       [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == b; }))
            name.resize(name.size() - ext.size());
    }
    return name;
}

static bool IsValidSceneName(const std::string& name)
{
    if (name.empty() || name == "." || name == "..")
        return false;
    return name.find_first_of("\\/:*?\"<>|") == std::string::npos;   // characters Windows forbids in file names
}

bool EngineClass::SceneExists(const std::string& fileName) const
{
    return std::any_of(m_sceneFiles.begin(), m_sceneFiles.end(), [&](const FileInfo& f) {
        return f.name.size() == fileName.size() &&
            std::equal(f.name.begin(), f.name.end(), fileName.begin(), [](char a, char b) {
            return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
        });
    });
}

void EngineClass::MainMenuBar()
{
    ImGui::DockSpaceOverViewport();
    if (ImGui::BeginMainMenuBar()) {
        bool openLoadPopup = false;

        if (ImGui::BeginMenu("File")) {
            const bool canEditScene = GetEngineMode() == EngineMode::Editing;
			if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, canEditScene)) {
				const std::string name = NormalizeSceneName(m_saveNameBuf);
				if (IsValidSceneName(name)) {
					const std::string fileName = name + ".json";
					const std::string path = GetResourceManager()->GetPath(
						ResourceManager::Directory::Assets, "scenes/" + fileName);
					GetECS()->GetSystem<SceneManager>().SaveSceneToFile(path);
				}
			}
            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S", false, canEditScene)) {
                m_sceneFiles = GetResourceManager()->GetFilesInDirectory(
                    ResourceManager::Directory::Assets, "scenes", ".json");
                m_openSceneSaver = true;
            }
            if (ImGui::MenuItem("Load Scene", "Ctrl+L", false, canEditScene)) {
                m_sceneFiles = GetResourceManager()->GetFilesInDirectory(
                    ResourceManager::Directory::Assets, "scenes", ".json");   // scan the disk once on open, not every frame
                m_openSceneLoader = true;
            }
            ImGui::EndMenu();
        }

        ModalPopup("Save Scene", m_openSceneSaver, ImVec2(400, 330), [&] {
            ImGui::TextUnformatted("Existing scenes (click to reuse a name):");
            ImGui::BeginChild("##existing", ImVec2(0, 150), true);
            for (const FileInfo& f : m_sceneFiles) {
                if (ImGui::Selectable(f.name.c_str())) {
                    const std::string stem = std::filesystem::path(f.name).stem().string();
                    std::snprintf(m_saveNameBuf, sizeof(m_saveNameBuf), "%s", stem.c_str());   // fills the field, doesn't save
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::TextUnformatted("File name:");
            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();
            const bool enterPressed = ImGui::InputText("##saveName", m_saveNameBuf, sizeof(m_saveNameBuf),
                                                       ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            ImGui::TextUnformatted(".json");

            const std::string name = NormalizeSceneName(m_saveNameBuf);
            const bool valid = IsValidSceneName(name);
            const std::string fileName = name + ".json";
            const bool exists = valid && SceneExists(fileName);

            if (!valid)
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Invalid name");
            else if (exists)
                ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "File exists, you'll be asked to confirm");

            ImGui::BeginDisabled(!valid);
            if (ImGui::Button("Save") || (enterPressed && valid)) {
                const std::string path = GetResourceManager()->GetPath(
                    ResourceManager::Directory::Assets, "scenes/" + fileName);
                if (exists) {
                    m_pendingSavePath = path;
                    m_openOverwriteConfirm = true;
                }
                else {
                    GetECS()->GetSystem<SceneManager>().SaveSceneToFile(path);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();

            if (m_closeSceneSaver) {                       // after the nested popup ends, we're back in the saver's scope
                m_closeSceneSaver = false;
                ImGui::CloseCurrentPopup();                // closes the saver
            }
        });

        ModalPopup("Overwrite?", m_openOverwriteConfirm, ImVec2(360, 110), [&] {
			std::string fileName = NormalizeSceneName(m_saveNameBuf) + ".json";
            ImGui::TextWrapped("%s already exists. Overwrite it?", fileName.c_str());
            if (ImGui::Button("Overwrite")) {
                GetECS()->GetSystem<SceneManager>().SaveSceneToFile(m_pendingSavePath);
                m_closeSceneSaver = true;
                ImGui::CloseCurrentPopup();            // closes the overwrite dialog
            }
            ImGui::SameLine();
            if (ImGui::Button("Back"))
                ImGui::CloseCurrentPopup();
        });

        ModalPopup("Scene Loader", m_openSceneLoader, ImVec2(400, 300), [&] {
            ImGui::TextUnformatted("Which scene would you like to load?");
            ImGui::Separator();

            for (const FileInfo& sceneFile : m_sceneFiles) {
                if (ImGui::Selectable(sceneFile.name.c_str())) {
                    GetECS()->GetSystem<SceneManager>().LoadSceneFromFile(sceneFile.path);
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();
        });

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
            if (ImGui::MenuItem("Save & Quit")) {
                GetECS()->GetSystem<SceneManager>().SaveSceneToFile(
                    GetResourceManager()->GetPath(ResourceManager::Directory::Assets, "scenes/example.json"));
				running = false;
            }
            /*if (ImGui::MenuItem("Quit", "Alt+F4"))
                running = false;*/
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

    m_frameCount++;

	if (m_frameCount % 60 == 0) {
        m_resourceManager->UnloadUnusedResources();
	}

    return true;
}
