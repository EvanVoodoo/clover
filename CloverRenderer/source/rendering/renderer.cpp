#include "rendering/renderer.hpp"
#include <random>

using namespace clvr;

Renderer::Renderer()
{
	m_DX2D = nullptr;
}

Renderer::Renderer(const Renderer& other) {

}

Renderer::~Renderer() {

}

bool Renderer::Initialize(int screenWidth, int screenHeight, HWND hwnd)
{
	bool result;


	// Create and initialize the Direct3D object.
	m_DX2D = new DirectX2D;

	result = m_DX2D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize Direct3D", L"Error", MB_OK);
		return false;
	}

	return true;
}

void Renderer::Shutdown()
{
	// Release the Direct3D object.
	if (m_DX2D)
	{
		m_DX2D->Shutdown();
		delete m_DX2D;
		m_DX2D = nullptr;
	}
	return;
}

bool Renderer::Frame(float dt)
{
	bool result;

	// Render the graphics scene.
	result = Render(dt);
	if (!result)
	{
		return false;
	}

	return true;
}

bool Renderer::Render(float dt)
{
	static std::mt19937 rng(std::random_device{}());
	static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	static float r = dist(rng), g = dist(rng), b = dist(rng);
	static float elapsed = 0.0f;

	elapsed += dt;
	if (elapsed >= 1.0f)
	{
		r = dist(rng);
		g = dist(rng);
		b = dist(rng);
		elapsed = 0.0f;
	}

	// Clear the buffers to begin the scene.
	m_DX2D->BeginScene(r, g, b, 1.0f);


	// Present the rendered scene to the screen.
	m_DX2D->EndScene();

	return true;
}