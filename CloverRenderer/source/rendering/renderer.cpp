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
	static std::mt19937 rng(std::random_device{}());
	static std::uniform_real_distribution<float> posDist(-300.0f, 300.0f);
	static std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
	static std::uniform_real_distribution<float> sizeDist(100.0f, 300.0f);

	static Sprite sprites[3];
	static float elapsed = 0.0f;

	elapsed += dt;
	if (elapsed >= 1.0f)
	{
		for (auto& sprite : sprites)
		{
			sprite.position = XMFLOAT2(posDist(rng), posDist(rng));
			sprite.size = XMFLOAT2(sizeDist(rng), sizeDist(rng));
			sprite.color = XMFLOAT4(colorDist(rng), colorDist(rng), colorDist(rng), 1.0f);
		}
		elapsed = 0.0f;
	}

	m_DX2D->BeginScene(0.1f, 0.1f, 0.1f, 1.0f);

	for (const auto& sprite : sprites)
		m_DX2D->DrawSprite(sprite);

	m_DX2D->EndScene();

	return true;
}