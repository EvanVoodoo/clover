#pragma once

#include "core/window.hpp"
#include "core/input.hpp"
#include "rendering/renderer.hpp"

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

	private:
		bool Frame(float dt);

		Input* m_input;
		Window* m_window;
		Renderer* m_renderer;
	};
}