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

	result = m_DX2D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN);
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

void Renderer::DrawSprite(const Sprite& sprite, const Transform& transform) { m_DX2D->DrawSprite(sprite, transform); }

bool Renderer::Render(float dt)
{
	m_DX2D->BeginScene(1.0f, 1.0f, 1.0f, 1.0f);

	UpdateLights();

	auto view = Engine.GetECS()->GetRegistry().view<SpriteComponent, Transform>();

	std::vector<std::pair<const Sprite*, const Transform*>> toDraw;
	toDraw.reserve(view.size_hint());

	for (auto [entity, sc, t] : view.each())
		toDraw.emplace_back(&sc.sprite, &t);

	std::stable_sort(toDraw.begin(), toDraw.end(),
		[](const auto& a, const auto& b) {
			return a.first->layer < b.first->layer;
		});

	for (const auto& [sprite, transform] : toDraw)
		DrawSprite(*sprite, *transform);


	m_DX2D->EndScene();
	return true;
}

void Renderer::UpdateLights()
{
	auto& registry = Engine.GetECS()->GetRegistry();
	BufferType::LightBufferType lightData = {};

	auto view = registry.view<Transform, Light>();
	int count = 0;
	for (auto [entity, transform, light] : view.each())
	{
		if (count >= MAX_LIGHTS)   // MAX_LIGHTS = your array size
		{
			assert(false && "More light entities than LightBufferType can hold");
			break;
		}

		lightData.lights[count] = light;
		++count;
	}
	lightData.lightCount = count;

	m_DX2D->UpdateLights(lightData);   // forwards to m_lightCb.Update
}

int Renderer::AddTexture(const wchar_t* filename) { return m_DX2D->AddTexture(filename); }
bool Renderer::BuildAtlas() { return m_DX2D->BuildAtlas(); }
AtlasRegion Renderer::GetAtlasRegion(const wchar_t* f) { return m_DX2D->GetAtlasRegion(f); }