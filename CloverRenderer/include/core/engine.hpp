#pragma once

#include "core/window.hpp"
#include "core/input.hpp"
#include "core/ecs.hpp"
#include "imgui_layer.hpp"

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

#ifdef CLOVER_EDITOR
	enum class EditorMode { Editing, Playing };
#endif

	class EngineClass
	{
	public:
		bool Initialize(HINSTANCE hInstance, int nCmdShow);
		void Shutdown();
		void Run();

		EntityComponentSystem* GetECS() { return m_ecs; }
		Window* GetWindow() { return m_window; }
		Input* GetInput() { return m_input; }
		ImGuiLayer* GetImGuiLayer() { return m_imgui; }

		bool running = false;

#ifdef CLOVER_EDITOR
		EditorMode GetEditorMode() const { return m_editorMode; }
		void SetEditorMode(EditorMode mode) { m_editorMode = mode; }
#endif

	private:
		bool Frame(float dt);

		EntityComponentSystem* m_ecs = nullptr;
		Input* m_input = nullptr;
		Window* m_window = nullptr;
		ImGuiLayer* m_imgui = nullptr;
#ifdef CLOVER_EDITOR
		EditorMode m_editorMode = EditorMode::Editing;
#endif
	};

	extern EngineClass Engine;
}