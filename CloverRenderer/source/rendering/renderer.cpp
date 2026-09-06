#include "rendering/renderer.hpp"
#include <random>
#include <core/engine.hpp>
#include <core/components.hpp>

using namespace clvr;

Renderer::Renderer()
{
	m_DX2D = nullptr;

	priority = -1;
	title = "Renderer";
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

	Window* window = Engine.GetWindow();
	window->SetResizeCallback([this](int w, int h)
	{
		if (m_DX2D)
			m_DX2D->UpdateWindowSize(static_cast<float>(w), static_cast<float>(h));
	});

	window->SetActivateWindowCallback([this]()
	{
		if (m_DX2D) 
			if (m_DX2D->IsFullscreen() != m_fullscreenMemory)
				m_DX2D->SetFullscreen(m_fullscreenMemory);
	});

	// Update the window size in the DirectX2D object one time during initialization to ensure it has the correct size.
	m_DX2D->UpdateWindowSize(static_cast<float>(window->GetWidth()), static_cast<float>(window->GetHeight()));

#ifdef CLOVER_EDITOR
	Engine.GetImGuiLayer()->Init(hwnd, m_DX2D->GetDevice(), m_DX2D->GetDeviceContext());
#endif

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
	m_DX2D->BeginScene(1.0f, 1.0f, 1.0f, 0.0f);

	UpdateLights();

	auto view = Engine.GetECS()->GetRegistry().view<SpriteComponent, Transform>();

	for (const auto& layer : m_spriteLayers)
	{
		std::vector<std::pair<const Sprite*, const Transform*>> toDraw;
		toDraw.reserve(view.size_hint());

		for (auto [entity, sc, t] : view.each())
		{
			if (sc.sprite.layer->id == layer->id)
				toDraw.emplace_back(&sc.sprite, &t);
		}

		m_DX2D->SetupLayer(*layer);

		for (const auto& [sprite, transform] : toDraw)
			DrawSprite(*sprite, *transform);

		m_DX2D->DrawLayer(*layer);
	}

#ifdef CLOVER_EDITOR
	ImGui::Begin("Game Scene", nullptr);

	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	m_DX2D->UpdateSceneWindowSize(viewportSize.x, viewportSize.y);
	ImGui::Image(m_DX2D->RenderScene(), viewportSize);

	ImGui::End();

	Engine.GetImGuiLayer()->EndFrame();
#else
	m_DX2D->RenderScene();
#endif

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

void Renderer::Inspect(float dt)
{
	ImGui::Begin("Graphics Settings");

	bool fullscreen = IsFullscreen();
	if (ImGui::Checkbox("Fullscreen", &fullscreen))
	{
		SetFullscreen(fullscreen);
	}

	ImGui::End();

	ImGui::Begin("Sprite Layers");

	if (ImGui::Button("Add Layer"))
	{
		SpriteLayer* layer = nullptr;
		int i = -1;
		while (true)
		{
			++i;
			layer = FindSpriteLayer(static_cast<unsigned int>(i));
			if (layer == nullptr)
				break;
		}
		CreateSpriteLayer(static_cast<unsigned int>(i), 1.0f);
	}

	ImGui::Separator();

	int layerToRemove = -1;
	int dragSrcIndex = -1;
	int dragDstIndex = -1;

	for (int i = 0; i < static_cast<int>(m_spriteLayers.size()); ++i)
	{
		SpriteLayer* layer = m_spriteLayers[i];
		ImGui::PushID(layer->id);

		// Drag handle
		ImGui::Selectable("::", false, 0, ImVec2(20, 0));
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
		{
			ImGui::SetDragDropPayload("SPRITE_LAYER_REORDER", &i, sizeof(int));
			ImGui::Text("Move %s", layer->layerName.c_str());
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE_LAYER_REORDER"))
			{
				dragSrcIndex = *static_cast<const int*>(payload->Data);
				dragDstIndex = i;
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();
		ImGui::Text("Layer %u", layer->id);
		ImGui::SameLine();
		if (ImGui::SmallButton("Remove"))
			layerToRemove = i;

		char nameBuf[128];
		strncpy_s(nameBuf, layer->layerName.c_str(), sizeof(nameBuf) - 1);
		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
			layer->layerName = nameBuf;

		ImGui::SliderFloat("Parallax Factor", &layer->parallaxFactor, 0.0f, 2.0f);

		ImGui::Separator();
		ImGui::PopID();
	}

	// Apply reorder after the loop, never mutate the vector mid-iteration
	if (dragSrcIndex != -1 && dragDstIndex != -1 && dragSrcIndex != dragDstIndex)
	{
		SpriteLayer* moved = m_spriteLayers[dragSrcIndex];
		m_spriteLayers.erase(m_spriteLayers.begin() + dragSrcIndex);
		m_spriteLayers.insert(m_spriteLayers.begin() + dragDstIndex, moved);
	}

	if (layerToRemove != -1)
	{
		// TODO: store layer íds so removing a layer and then adding one, doesn't cause issues with sprites still referencing the old layer
		delete m_spriteLayers[layerToRemove];
		m_spriteLayers.erase(m_spriteLayers.begin() + layerToRemove);
	}

	ImGui::End();
}

int Renderer::AddTexture(const wchar_t* filename) { return m_DX2D->AddTexture(filename); }
bool Renderer::BuildAtlas() { return m_DX2D->BuildAtlas(); }
AtlasRegion Renderer::GetAtlasRegion(const wchar_t* f) { return m_DX2D->GetAtlasRegion(f); }

SpriteLayer* Renderer::CreateSpriteLayer(const unsigned int id, float parallaxFactor, const std::string& layerName)
{
	SpriteLayer* newLayer = new SpriteLayer();
	newLayer->id = id;
	if (layerName.empty())
		newLayer->layerName = "Sprite Layer " + std::to_string(id);
	else
		newLayer->layerName = layerName;
	newLayer->parallaxFactor = parallaxFactor;

	m_spriteLayers.push_back(newLayer);
	return newLayer;
}

SpriteLayer* Renderer::FindSpriteLayer(unsigned int id)
{
	auto it = std::find_if(m_spriteLayers.begin(), m_spriteLayers.end(),
						   [id](SpriteLayer* l) { return l->id == id; });

	if (it != m_spriteLayers.end())
		return *it;

	return nullptr;
}

SpriteLayer* Renderer::FindOrCreateSpriteLayer(unsigned int id)
{
	auto it = std::find_if(m_spriteLayers.begin(), m_spriteLayers.end(),
						   [id](SpriteLayer* l) { return l->id == id; });

	if (it != m_spriteLayers.end())
		return *it;

	return CreateSpriteLayer(id, 1.0f);
}