#include "game.hpp"
#include "core/engine.hpp"
#include "core/components.hpp" 
#include "rendering/renderer.hpp"
#include <string>
#include <input/input_controller.hpp>

using namespace clvr;
using Dir = ResourceManager::Directory;

Game::Game() {
	priority = 10;
	title = "GameSystem";
	SetupScene();
}

void Game::SetupScene()
{
	auto ecs = Engine.GetECS();
	auto& renderer = ecs->GetSystem<Renderer>();
	ResourceManager* rMngr = Engine.GetResourceManager();
	std::string textures[] = {
		rMngr->GetPath(Dir::SharedAssets, "textures/shrew1.jpg"),
		rMngr->GetPath(Dir::SharedAssets, "textures/shrew2.jpg"),
		rMngr->GetPath(Dir::SharedAssets, "textures/hamper.jpeg"),
		rMngr->GetPath(Dir::SharedAssets, "textures/white.jpg"),
	};
	for (auto t : textures)
		renderer.AddTexture(t);
	renderer.BuildAtlas();   // built ONCE, after all textures added, before first frame

	{
		auto centerEntity = ecs->CreateEntity();
		auto& t = ecs->CreateComponent<Transform>(centerEntity);
		t.position = { 0.0f, -300.0f };
		t.name = "Player";
		clvr::Sprite cs = Sprite(rMngr->GetPath(Dir::SharedAssets, "textures/saturn.png"), true);
		cs.position = t.position;
		cs.size = { 200.0f, 200.0f };
		SpriteLayer* layer = renderer.FindOrCreateSpriteLayer(1);
		cs.layer = renderer.FindOrCreateSpriteLayer(1);
		cs.isOccluder = false;
		ecs->CreateComponent<SpriteComponent>(centerEntity, cs);
		auto& player = ecs->CreateComponent<PlayerComponent>(centerEntity);
		player.speed = 400.0f;
		ecs->CreateComponent<Controlled>(centerEntity, ControllerType::Player);
	}

	{
		// create wall of occluder sprites randomly around the scene
		for (int i = 0; i < 320; ++i)
		{
			auto entity = ecs->CreateEntity();
			auto& t = ecs->CreateComponent<Transform>(entity);
			t.position = { (float) (rand() % 32000 - 16000), (float) (rand() % 16000 - 8000) };
			t.rotation = (float) (rand() % 360) * 3.1415927f / 180.0f;
			t.name = "Occluder " + std::to_string(i);
			clvr::Sprite s;
			s.position = t.position;
			s.size = { (float) (rand() % 200 + 100), (float) (rand() % 100 + 50) };
			s.uvRect = renderer.GetAtlasRegion(rMngr->GetPath(Dir::SharedAssets, "textures/white.jpg")).uvRect;
			s.textureName = "textures/white.jpg";
			s.layer = renderer.FindOrCreateSpriteLayer(0);
			s.isOccluder = true;
			ecs->CreateComponent<SpriteComponent>(entity, s);
		}
		{
			auto entity = ecs->CreateEntity();
			auto& t = ecs->CreateComponent<Transform>(entity);
			t.position = { 0, 0 };
			t.name = "Background";
			clvr::Sprite s;
			s.position = t.position;
			s.size = { 3000, 3000 };
			s.uvRect = renderer.GetAtlasRegion(rMngr->GetPath(Dir::SharedAssets, "textures/shrew1.jpg")).uvRect;
			s.textureName = "textures/shrew1.jpg";
			SpriteLayer* layer = renderer.FindOrCreateSpriteLayer(3);
			s.layer = layer;
			s.isOccluder = false;
			ecs->CreateComponent<SpriteComponent>(entity, s);
		}
		// create ground plane occluder
		{
			auto entity = ecs->CreateEntity();
			auto& t = ecs->CreateComponent<Transform>(entity);
			t.position = { 0.0f, -400.0f };
			t.name = "Ground Occluder";
			clvr::Sprite s;
			s.position = t.position;
			s.size = { 16000.0f, 64.0f };
			s.uvRect = renderer.GetAtlasRegion(rMngr->GetPath(Dir::SharedAssets, "textures/white.jpg")).uvRect;
			s.textureName = "textures/white.jpg";
			s.layer = renderer.FindOrCreateSpriteLayer(0);
			s.isOccluder = true;
			ecs->CreateComponent<SpriteComponent>(entity, s);
		}
	}

	{ // create a single directional light pointing downwards
		auto lightEntity = ecs->CreateEntity();
		auto& transform = ecs->CreateComponent<Transform>(lightEntity);
		transform.name = "Directional Light";
		auto& light = ecs->CreateComponent<Light>(lightEntity);
		light.color = { 1.0f, 1.0f, 1.0f };
		light.direction = { 0.2f, -1.0f, 0.0f };
		light.intensity = .25f;
		light.type = 0; // directional light
	}

	{
		// varied point lights randomly positioned across the scene
		const int count = 4;
		for (int i = 0; i < count; ++i)
		{
			float t = (float) i * i * 2.17f / count;

			auto lightEntity = ecs->CreateEntity();
			auto& transform = ecs->CreateComponent<Transform>(lightEntity);
			transform.name = "Point Light " + std::to_string(i);
			auto& light = ecs->CreateComponent<Light>(lightEntity);
			ecs->CreateComponent<MovingLight>(lightEntity);

			// deterministic pseudo-random base position, seeded by index
			// (kept in sync with the wander logic in Update())
			float seedX = sinf((float) i * 12.9898f) * 43758.5453f;
			float seedY = sinf((float) i * 78.233f) * 43758.5453f;
			float baseX = fmodf(seedX, 1.0f) * 1600.0f - 800.0f;
			float baseY = fmodf(seedY, 1.0f) * 800.0f - 400.0f;

			transform.position = { baseX, baseY };
			light.direction = { transform.position.x, transform.position.y, 0.0f };

			float angle = t * 2.0f * 3.1415927f;
			light.color = {
				0.5f + 0.5f * cosf(angle),
				0.5f + 0.5f * cosf(angle + 2.094f),
				0.5f + 0.5f * cosf(angle + 4.188f)
			};

			light.intensity = 200.0f + t * 1000.0f;
			light.type = 1;
		}
	}
}

void Game::Update(float dt) {
	m_time += dt;
}

void Game::Render() {}

void Game::Inspect(float dt) {
	
}

void Game::OnPlayStart() {
	// Game camera goes in after the load, so nothing can clear it or take its ID
	const entt::entity camEntity = Engine.GetECS()->CreateEntity();
	auto& transform = Engine.GetECS()->CreateComponent<Transform>(camEntity);
	transform.name = "Game Camera";
	auto& camera = Engine.GetECS()->CreateComponent<CameraComponent>(camEntity);

	for (auto [e, c, p] : Engine.GetECS()->GetRegistry().view<Controlled, PlayerComponent>().each()) {
		if (c.type != ControllerType::Player) continue;
		camera.followEntity = e;
		camera.followTarget = true;
		break;                                     // first player only
	}
}