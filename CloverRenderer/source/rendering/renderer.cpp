#include "rendering/renderer.hpp"

#include "rendering/render_d3d11.hpp"
#include "rendering/render_components.hpp"
#include <core/engine.hpp>
#include <core/scene.hpp>

using namespace clvr;

Renderer::Renderer()
{
	m_DX2D = nullptr;

	priority = -1;
	title = "Renderer";
}

Renderer::~Renderer() {

}

bool Renderer::Initialize(int screenWidth, int screenHeight)
{
	bool result;

	// Create and initialize the Direct3D object.
	m_DX2D = new DirectX2D();

	result = m_DX2D->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, FULL_SCREEN);
	if (!result)
	{
		MessageBox(Engine.GetWindow()->GetHWND(), L"Could not initialize Direct3D", L"Error", MB_OK);
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
	Engine.GetImGuiLayer()->Init(Engine.GetWindow()->GetHWND(), m_DX2D->GetDevice(), m_DX2D->GetDeviceContext());
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
	// Render the graphics scene.
	EditorWindowControls(dt);
}

void clvr::Renderer::Render()
{
	m_DX2D->BeginScene(1.0f, 1.0f, 1.0f, 0.0f);

	UpdateLights();

	auto view = Engine.GetECS()->GetRegistry().view<SpriteComponent, Transform>();

	for (const auto& layer : m_spriteLayers)
	{
		std::vector<std::pair<const Sprite*, const Transform*>> batched;
		std::vector<std::pair<const Sprite*, const Transform*>> unbatched;
		batched.reserve(view.size_hint());

		for (auto [entity, sc, t] : view.each())
		{
			if (sc.sprite.layer->id != layer->id)
				continue;

			if (sc.sprite.useLinkedTexture)
				unbatched.emplace_back(&sc.sprite, &t);
			else
				batched.emplace_back(&sc.sprite, &t);
		}

		// Unbatched pass — same layer, so it needs the same parallax-adjusted view
		for (const auto& [sprite, transform] : unbatched)
			m_DX2D->DrawUnbatchedSprite(*sprite, *transform, *layer);

		// Atlas-batched pass
		m_DX2D->SetupLayer(*layer);
		for (const auto& [sprite, transform] : batched)
			m_DX2D->DrawSprite(*sprite, *transform, *layer, GetActiveCamera().transform);
		m_DX2D->DrawLayer(*layer);
	}

#ifdef CLOVER_EDITOR
	ImGui::Begin("Game Scene", nullptr);
	m_gameWindowFocused = ImGui::IsWindowFocused();

	ImVec2 viewportPos = ImGui::GetCursorScreenPos();
	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	m_DX2D->UpdateSceneWindowSize(viewportSize.x, viewportSize.y);
	ImGui::Image(m_DX2D->RenderScene(), viewportSize);

	// ID buffer rendering for picking
	m_DX2D->BeginIDPass();
	for (const auto& layer : m_spriteLayers)
	{
		for (auto [entity, sc, t] : view.each())
		{
			if (sc.sprite.layer->id != layer->id)
				continue;
			m_DX2D->DrawSpriteID(sc.sprite, t, *layer, static_cast<uint32_t>(entity));
		}
	}
	m_DX2D->EndIDPass();

	// Handle mouse picking
	if (m_gameWindowFocused && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		// TODO: Scaling not 100% accurate, double check the math here.
		// mouse pos should be relative to the game scene viewport, not the whole window
		ImVec2 mousePos = ImGui::GetMousePos();
		int mouseX = static_cast<int>(mousePos.x - viewportPos.x);
		int mouseY = static_cast<int>(mousePos.y - viewportPos.y);
		
		// convert mouse coordinates to 0-1 range
		XMFLOAT2 mouseNorm = XMFLOAT2(static_cast<float>(mouseX) / viewportSize.x, static_cast<float>(mouseY) / viewportSize.y);
		
		// convert clicked mouse coordinates to pixel coordinates in the ID buffer using aspect ratio and viewport size
		float screenRatio = viewportSize.x / viewportSize.y;
		float targetRatio = m_DX2D->GetAspectRatio();

		bool isLetterbox = screenRatio < targetRatio;

		XMFLOAT2 scale = isLetterbox ? XMFLOAT2(1.0f, targetRatio / screenRatio) : XMFLOAT2(screenRatio / targetRatio, 1.0f);

		XMFLOAT2 scaledMouseNorm = XMFLOAT2((mouseNorm.x - 0.5f) * scale.x + 0.5f, (mouseNorm.y - 0.5f) * scale.y + 0.5f);

		if (mouseX >= 0 && mouseY >= 0 && mouseX < static_cast<int>(viewportSize.x) && mouseY < static_cast<int>(viewportSize.y))
		{
			Engine.GetECS()->GetSystem<SceneManager>().UpdateSelectedEntity(entt::null); // Deselect any previously selected entity
			auto pickedEntity = m_DX2D->PickEntityAtNormalizedCoords(scaledMouseNorm.x, scaledMouseNorm.y);
			uint32_t entityID = -1;
			if (pickedEntity.has_value())
			{
				entityID = pickedEntity.value();
			}
			if (entityID != 0)
			{
				Entity selectedEntity = static_cast<Entity>(entityID);
				auto& sceneManager = Engine.GetECS()->GetSystem<SceneManager>();
				sceneManager.UpdateSelectedEntity(selectedEntity);
				sceneManager.SetScrollEntity(selectedEntity);
			}
			else
			{
				Engine.GetECS()->GetSystem<SceneManager>().UpdateSelectedEntity(entt::null); // Deselect if clicked on empty space
			}
		}
	}

	// TODO: Draw gizmos here, after the scene is rendered but before ImGui::End() so they appear on top of the scene
	Engine.GetECS()->GetSystem<SceneManager>().Draw();

	// Rebind the back buffer so the ImGui backend's own draw calls
	// (ImGui_ImplDX11_RenderDrawData) land in the swapchain, not m_finalFramebuffer.
	m_DX2D->SetBackBufferRenderTarget();
	ImGui::End();

	Engine.GetImGuiLayer()->EndFrame();
#else
	m_DX2D->RenderScene();
#endif

	m_DX2D->EndScene();
}

void Renderer::DrawSprite(const Sprite& sprite, const Transform& transform, const SpriteLayer& layer, const Transform& cameraTransform) { m_DX2D->DrawSprite(sprite, transform, layer, cameraTransform); }

void Renderer::DrawUnbatchedSprite(const Sprite& sprite, const Transform& transform, const SpriteLayer& layer) { m_DX2D->DrawUnbatchedSprite(sprite, transform, layer); }

void Renderer::SetActiveShader(const std::wstring& name) { m_DX2D->SetActiveShader(name); }

void Renderer::SetPostProcessShader(const std::wstring& name) { m_DX2D->SetPostProcessShader(name); }

bool Renderer::LoadShader(const std::string& name, const std::string& vsFilename, const std::string& psFilename)
{
	return m_DX2D->LoadShader(name, vsFilename, psFilename);
}

bool Renderer::ReloadShaders() { return m_DX2D->ReloadShaders(); }

Camera& Renderer::GetActiveCamera() { return m_DX2D->GetActiveCamera(); }

inline bool Renderer::SetFullscreen(bool fullscreen) {
	bool result = m_DX2D->SetFullscreen(fullscreen);
	if (result)
		m_fullscreenMemory = fullscreen;
	return result;
}

inline bool Renderer::IsFullscreen() const { return m_DX2D->IsFullscreen(); }

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
		// work around the fact that directional lights don't have a position, but point lights do, so we use the position of the transform for point lights
		// creating temp Light variable to modify the direction for point lights, without modifying the original Light component in the ECS
		Light l = light;
		if (l.type != 0.0f)   // Directional lights need a direction, while point lights use position
		{
			l.direction = XMFLOAT3(transform.position.x, transform.position.y, 0.5f);   // For point lights, use position instead of direction
		}
		lightData.lights[count] = l;
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

void* Renderer::GetNativeDeviceHandle() { return static_cast<void*>(m_DX2D->GetDevice()); }

int Renderer::AddTexture(const std::string filename) { return m_DX2D->AddTexture(filename); }

std::shared_ptr<Texture> Renderer::LoadTexture(std::string filename)
{
	return m_DX2D->LoadTexture(filename);
}

bool Renderer::BuildAtlas() { return m_DX2D->BuildAtlas(); }

AtlasRegion Renderer::GetAtlasRegion(const std::string f) { return m_DX2D->GetAtlasRegion(f); }

TextureAtlas* clvr::Renderer::GetTextureAtlas()
{
	return m_DX2D->GetTextureAtlas();
}

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

void Renderer::EditorWindowControls(float dt)
{
	if (!m_gameWindowFocused) return;

	if (Engine.GetEditorMode() == EditorMode::Playing) return;

	auto ecs = Engine.GetECS();
	auto input = Engine.GetInput();

	if (input->IsKeyDown('1'))
		SetActiveShader(L"default");
	else if (input->IsKeyDown('2'))
		SetActiveShader(L"grayscale");
	else if (input->IsKeyDown('3'))
		SetActiveShader(L"inverted");
	else if (input->IsKeyDown('4'))
		SetActiveShader(L"chromatic");
	else if (input->IsKeyDown('5'))
		SetActiveShader(L"wacky");

	if (input->IsKeyDown('R'))
	{
		ReloadShaders();
	}

	// move camera with arrow keys
	Camera& camera = GetActiveCamera();
	float cameraSpeedMult = 1.0f;

	if (input->IsKeyDown(VK_SHIFT))
		cameraSpeedMult *= 10.0f;
	if (input->IsKeyDown(VK_CONTROL))
		cameraSpeedMult *= 0.1f;
	float cameraSpeed = camera.speed * cameraSpeedMult * dt;

	if (input->IsKeyDown('Q'))
		camera.transform.rotation += (cameraSpeed * 0.1f * 2 * 3.1415927f) / 180.f;
	if (input->IsKeyDown('E'))
		camera.transform.rotation -= (cameraSpeed * 0.1f * 2 * 3.1415927f) / 180.f;

	// zoom in/out with W/S keys by adjusting the camera's projection matrix
	if (input->IsKeyDown('W'))
	{
		camera.zoom = camera.zoom * 1.01f;
		//if (camera.zoom > 5.0f)
		//camera.zoom = 5.0f;
	}
	if (input->IsKeyDown('S'))
	{
		camera.zoom = camera.zoom * 0.99f;
		//camera.zoom -= cameraSpeed * 0.01f;
		if (camera.zoom < 0.01f)
			camera.zoom = 0.01f;
	}

	// camera moves according to its rotation
	if (input->IsKeyDown(VK_UP))
	{
		camera.transform.position.x -= sinf(camera.transform.rotation) * cameraSpeed;
		camera.transform.position.y += cosf(camera.transform.rotation) * cameraSpeed;
	}
	if (input->IsKeyDown(VK_DOWN))
	{
		camera.transform.position.x += sinf(camera.transform.rotation) * cameraSpeed;
		camera.transform.position.y -= cosf(camera.transform.rotation) * cameraSpeed;
	}
	if (input->IsKeyDown(VK_LEFT))
	{
		camera.transform.position.x -= cosf(camera.transform.rotation) * cameraSpeed;
		camera.transform.position.y -= sinf(camera.transform.rotation) * cameraSpeed;
	}
	if (input->IsKeyDown(VK_RIGHT))
	{
		camera.transform.position.x += cosf(camera.transform.rotation) * cameraSpeed;
		camera.transform.position.y += sinf(camera.transform.rotation) * cameraSpeed;
	}

	if (input->IsKeyDown('C'))
	{
		camera.transform = Transform();
		camera.zoom = 1.0f;
		camera.nearZ = 0.0f;
		camera.farZ = 1.0f;
	}
}