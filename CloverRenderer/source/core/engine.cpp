#include "core/engine.hpp"
#include <chrono>

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

    return true;
}

void EngineClass::Shutdown()
{
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

			GetECS()->UpdateSystems(deltaTime);

            result = Frame(deltaTime);
            if (!result)
            {
                running = false;
            }
        }
    }
}

bool EngineClass::Frame(float dt)
{
    //bool result;

    // Check if the user pressed escape and wants to exit the application.
    if (m_input->IsKeyDown(VK_ESCAPE))
    {
        return false;
    }

 //   if (m_input->IsKeyDown('1'))
 //       m_renderer->SetActiveShader(L"default");
 //   else if (m_input->IsKeyDown('2'))
 //       m_renderer->SetActiveShader(L"grayscale");
 //   else if (m_input->IsKeyDown('3'))
 //       m_renderer->SetActiveShader(L"inverted");
 //   else if (m_input->IsKeyDown('4'))
 //       m_renderer->SetActiveShader(L"chromatic");
	//else if (m_input->IsKeyDown('5'))
	//	m_renderer->SetActiveShader(L"wacky");

	//if (m_input->IsKeyDown('R'))
	//{
	//	m_renderer->ReloadShaders();
	//}

	//// move camera with arrow keys
	//Camera& camera = m_renderer->GetCamera();
	//float cameraSpeedMult = 1.0f;

	//if (m_input->IsKeyDown(VK_SHIFT))
	//	cameraSpeedMult = 10.0f;
	//if (m_input->IsKeyDown(VK_CONTROL))
	//	cameraSpeedMult = 0.1f;
	//float cameraSpeed = camera.speed * cameraSpeedMult * dt;

 //   if (m_input->IsKeyDown('Q'))
 //       camera.transform.rotation += (cameraSpeed * 0.1f * 2 * 3.1415927f) / 180.f;
 //   if (m_input->IsKeyDown('E'))
 //       camera.transform.rotation -= (cameraSpeed * 0.1f * 2 * 3.1415927f) / 180.f;

	//// camera moves according to its rotation
 //   if (m_input->IsKeyDown(VK_UP))
	//{
	//	camera.transform.position.x -= sinf(camera.transform.rotation) * cameraSpeed;
	//	camera.transform.position.y += cosf(camera.transform.rotation) * cameraSpeed;
	//}
 //   if (m_input->IsKeyDown(VK_DOWN))
 //   {
	//	camera.transform.position.x += sinf(camera.transform.rotation) * cameraSpeed;
	//	camera.transform.position.y -= cosf(camera.transform.rotation) * cameraSpeed;
 //   }
 //   if (m_input->IsKeyDown(VK_LEFT))
 //   {
	//	camera.transform.position.x -= cosf(camera.transform.rotation) * cameraSpeed;
	//	camera.transform.position.y -= sinf(camera.transform.rotation) * cameraSpeed;
 //   }
 //   if (m_input->IsKeyDown(VK_RIGHT))
 //   {
 //       camera.transform.position.x += cosf(camera.transform.rotation) * cameraSpeed;
 //       camera.transform.position.y += sinf(camera.transform.rotation) * cameraSpeed;
 //   }

	//if (m_input->IsKeyDown('C'))
	//	camera.transform = Transform();

    return true;
}
