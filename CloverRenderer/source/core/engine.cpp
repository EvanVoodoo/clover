#include "core/engine.hpp"
#include <chrono>

using namespace clvr;

Engine::Engine()
{
	m_renderer = nullptr;
	m_window = nullptr;
}

Engine::Engine(const Engine&)
{

}

Engine::~Engine()
{

}

bool Engine::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    m_window = new Window();
    if (!m_window->Initialize(hInstance, nCmdShow, SCREEN_WIDTH, SCREEN_HEIGHT))
        return false;

    m_renderer = new Renderer();
    if (!m_renderer->Initialize(m_window->GetWidth(), m_window->GetHeight(), m_window->GetHWND()))
        return false;

    return true;
}

void Engine::Shutdown()
{
	if (m_renderer)
	{
		m_renderer->Shutdown();
		delete m_renderer;
		m_renderer = nullptr;
	}

    if (m_window)
    {
        m_window->Shutdown();
        delete m_window;
        m_window = nullptr;
    }
}

void Engine::Run()
{
    MSG msg;
    bool running = true;

    auto lastTime = std::chrono::steady_clock::now();

    while (running)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }
        else
        {
            auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;

            Frame(deltaTime);
        }
    }
}

bool Engine::Frame(float dt)
{
	return m_renderer->Frame(dt);
}
