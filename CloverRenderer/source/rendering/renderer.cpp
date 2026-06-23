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

void Renderer::DrawSprite(const Sprite& sprite) { m_DX2D->DrawSprite(sprite); }

bool Renderer::Render(float dt)
{
	// Clear the buffers to begin the scene.
	m_DX2D->BeginScene(0.1f, 0.1f, 0.1f, 1.0f);

	// Submit sprites here
	{
		Sprite sprite;
		sprite.position = XMFLOAT2(0.0f, 0.0f);
		sprite.size = XMFLOAT2(200.0f, 200.0f);
		sprite.color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
		m_DX2D->DrawSprite(sprite);
	}

	{
		Sprite sprite;
		sprite.position = XMFLOAT2(250.0f, 0.0f);
		sprite.size = XMFLOAT2(200.0f, 200.0f);
		sprite.color = XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f);
		m_DX2D->DrawSprite(sprite);
	}

	{
		Sprite sprite;
		sprite.position = XMFLOAT2(-100.0f, 0.0f);
		sprite.size = XMFLOAT2(200.0f, 200.0f);
		sprite.color = XMFLOAT4(0.0f, 1.0f, 1.0f, 0.1f);
		m_DX2D->DrawSprite(sprite);
	}

	// Present the rendered scene to the screen.
	m_DX2D->EndScene();

	return true;
}