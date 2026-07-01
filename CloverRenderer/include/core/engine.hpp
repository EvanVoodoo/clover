#pragma once

#include "core/window.hpp"
#include "core/input.hpp"
#include "rendering/renderer.hpp"
#include "core/ecs.hpp"

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

namespace clvr
{
	class EngineClass
	{
	public:
		bool Initialize(HINSTANCE hInstance, int nCmdShow);
		void Shutdown();
		void Run();

		EntityComponentSystem* GetECS() { return m_ecs; }
		Window* GetWindow() { return m_window; }
		Input* GetInput() { return m_input; }

		bool running = false;

	private:
		bool Frame(float dt);

		EntityComponentSystem* m_ecs = nullptr;
		Input* m_input = nullptr;
		Window* m_window = nullptr;
	};

	extern EngineClass Engine;
}