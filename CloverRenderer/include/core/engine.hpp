#pragma once

#include "core/window.hpp"
#include "core/input.hpp"
#include "rendering/renderer.hpp"
#include "core/ecs.hpp"

const int SCREEN_WIDTH = 1600;
const int SCREEN_HEIGHT = 900;

namespace clvr
{
	class Engine
	{
	public:
		Engine();
		Engine(const Engine&);
		~Engine();

		bool Initialize(HINSTANCE hInstance, int nCmdShow);
		void Shutdown();
		void Run();

		EntityComponentSystem* GetECS() { return m_ecs; }

	private:
		bool Frame(float dt);

		EntityComponentSystem* m_ecs;
		Input* m_input;
		Window* m_window;
		Renderer* m_renderer;
	};
}