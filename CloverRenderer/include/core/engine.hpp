#pragma once

#include "core/window.hpp"
#include "rendering/renderer.hpp"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

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

		//InputClass* m_Input;
		Window* m_window;
		Renderer* m_renderer;
	};
}