#include "core/engine.hpp"
#include <chrono>

using namespace clvr;

Engine::Engine()
{
	m_renderer = nullptr;
	m_window = nullptr;
    m_input = nullptr;
}

Engine::Engine(const Engine&)
{

}

Engine::~Engine()
{

}

bool Engine::Initialize(HINSTANCE hInstance, int nCmdShow)
{
	m_input = new Input();
	if (!m_input->Initialize())
		return false;

    m_window = new Window();
    if (!m_window->Initialize(hInstance, nCmdShow, SCREEN_WIDTH, SCREEN_HEIGHT, m_input))
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

	if (m_input)
	{
        m_input->Shutdown();
		delete m_input;
        m_input = nullptr;
	}
}

void Engine::Run()
{
    MSG msg;
    bool running = true, result;

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

            result = Frame(deltaTime);
            if (!result)
            {
                running = false;
            }
        }
    }
}

bool Engine::Frame(float dt)
{
    bool result;


    // Check if the user pressed escape and wants to exit the application.
    if (m_input->IsKeyDown(VK_ESCAPE))
    {
        return false;
    }

    result = m_renderer->Frame(dt);
    if (!result)
    {
        return false;
    }

    return true;
}
