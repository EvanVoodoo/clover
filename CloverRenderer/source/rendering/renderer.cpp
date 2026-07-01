#include "rendering/renderer.hpp"
#include <random>
#include <core/engine.hpp>
#include <core/components.hpp>

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

void Renderer::Update(float dt)
{
    Frame(dt);
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
	m_DX2D->BeginScene(0.1f, 0.1f, 0.1f, 1.0f);

	auto view = Engine.GetECS()->GetRegistry().view<SpriteComponent>();

	// Collect for layer sort. view iteration order isn't layer order,
	// and your batcher needs back-to-front, so gather then sort.
	std::vector<const Sprite*> toDraw;
	toDraw.reserve(view.size());
	for (auto entity : view)
		toDraw.push_back(&view.get<SpriteComponent>(entity).sprite);

	std::stable_sort(toDraw.begin(), toDraw.end(),
		[](const Sprite* a, const Sprite* b) { return a->layer < b->layer; });

	for (const Sprite* s : toDraw)
		DrawSprite(*s);

	m_DX2D->EndScene();
	return true;
}

int Renderer::AddTexture(const wchar_t* filename) { return m_DX2D->AddTexture(filename); }
bool Renderer::BuildAtlas() { return m_DX2D->BuildAtlas(); }
AtlasRegion Renderer::GetAtlasRegion(const wchar_t* f) { return m_DX2D->GetAtlasRegion(f); }