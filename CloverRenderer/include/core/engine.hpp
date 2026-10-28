#pragma once

#include "core/window.hpp"
#include "input/input.hpp"
#include "core/ecs.hpp"
#include "imgui_layer.hpp"
#include "resources/resource_manager.hpp"
#include "core/common.hpp"

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

namespace clvr
{
	bool IsMouseMoving();
	bool IsKeyDown(unsigned int keycode);
	bool IsMouseButtonDown(unsigned int button);
	void GetMousePosition(int& x, int& y);
	bool WasKeyJustReleased(unsigned int key);
	bool WasKeyJustPressed(unsigned int key);
	bool WasMouseButtonJustReleased(unsigned int button);
	bool WasMouseButtonJustPressed(unsigned int button);
	const std::wstring ToWString(const std::string& str);
	bool IsDevEnvironment();

	class EngineClass
	{
	public:
		bool Initialize(HINSTANCE hInstance, int nCmdShow);
		void Shutdown();
		void Run();

		bool SceneExists(const std::string& fileName) const;
		void MainMenuBar();

		EntityComponentSystem* GetECS() { return m_ecs; }
		EntityComponentSystem& GetECSRef() { return *GetECS(); }
		Window* GetWindow() { return m_window; }
		Input* GetInput() { return m_input; }
		ResourceManager* GetResourceManager() { return m_resourceManager; }
		ImGuiLayer* GetImGuiLayer() { return m_imgui; }

		EngineMode GetEngineMode() const { return m_editorMode; }
		void SetEngineMode(EngineMode mode);

		bool GameWindowFocused();

		bool running = false;
	private:
		bool Frame(float dt);

		EntityComponentSystem* m_ecs = nullptr;
		Input* m_input = nullptr;
		Window* m_window = nullptr;
		ImGuiLayer* m_imgui = nullptr;
		ResourceManager* m_resourceManager = nullptr;
		EngineMode m_editorMode = EngineMode::Editing;
		unsigned long long m_frameCount = 0;

		std::vector<FileInfo> m_sceneFiles = {};

		bool m_openSceneSaver = false;
		bool m_openOverwriteConfirm = false;
		bool m_closeSceneSaver = false;       // lets the overwrite dialog close its parent too
		char m_saveNameBuf[128] = "untitled";
		std::string m_pendingSavePath;

		bool m_openSceneLoader = false;
	};

	extern EngineClass Engine;
}