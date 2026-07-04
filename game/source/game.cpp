#include "game.hpp"
#include "core/engine.hpp"
#include "core/components.hpp" 
#include "rendering/renderer.hpp"

using namespace clvr;

Game::Game() {
    SetupRenderer();
    SetupScene();
}

void Game::SetupRenderer()
{
    auto ecs = Engine.GetECS();
    auto& renderer = ecs->GetSystem<Renderer>();
    renderer.Initialize(Engine.GetWindow()->GetWidth(), Engine.GetWindow()->GetHeight(), Engine.GetWindow()->GetHWND());

    renderer.LoadShader(L"grayscale", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/grayscale.ps.hlsl");
    renderer.LoadShader(L"inverted", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/inverted.ps.hlsl");
    renderer.LoadShader(L"chromatic", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/chromatic.ps.hlsl");
    renderer.LoadShader(L"wacky", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/wacky.ps.hlsl");
    renderer.LoadShader(L"crt", L"../CloverRenderer/assets/shaders/post.vs.hlsl", L"../CloverRenderer/assets/shaders/crt.ps.hlsl");
    //renderer.SetPostProcessShader(L"crt");
}

void Game::SetupScene()
{
    auto ecs = Engine.GetECS();
    auto& renderer = ecs->GetSystem<Renderer>();

    const wchar_t* textures[] = {
        L"../CloverRenderer/assets/textures/shrew1.jpg",
        L"../CloverRenderer/assets/textures/shrew2.jpg",
        L"../CloverRenderer/assets/textures/hamper.jpeg",
        L"../CloverRenderer/assets/textures/saturn.png",
        L"../CloverRenderer/assets/textures/white.jpg",
    };
    for (auto* t : textures)
        renderer.AddTexture(t);
    renderer.BuildAtlas();   // built ONCE, after all textures added, before first frame

    /*{
        auto centerEntity = ecs->CreateEntity();
        auto& t = ecs->CreateComponent<Transform>(centerEntity);
        t.position = { 0.0f, -300.0f };
        clvr::Sprite cs;
        cs.position = t.position;
        cs.size = { 200.0f, 200.0f };
        cs.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        cs.uvRect = renderer.GetAtlasRegion(L"../CloverRenderer/assets/textures/shrew1.jpg").uvRect;
        cs.layer = 0;
        cs.isOccluder = false;
        ecs->CreateComponent<SpriteComponent>(centerEntity, cs);
        ecs->CreateComponent<MovingSprite>(centerEntity);
    }*/

    {
		// create wall of occluder sprites randomly around the scene
		for (int i = 0; i < 10; ++i)
		{
			auto entity = ecs->CreateEntity();
			auto& t = ecs->CreateComponent<Transform>(entity);
			t.position = { (float) (rand() % 1600 - 800), (float) (rand() % 800 - 400) };
			t.rotation = (float) (rand() % 360) * 3.1415927f / 180.0f;
			clvr::Sprite s;
			s.position = t.position;
			s.size = { (float) (rand() % 100 + 50), (float) (rand() % 100 + 50) };
			s.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            s.uvRect = renderer.GetAtlasRegion(L"../CloverRenderer/assets/textures/white.jpg").uvRect;
			s.layer = rand() % 5;
			s.isOccluder = true;
			ecs->CreateComponent<SpriteComponent>(entity, s);
		}
		// create ground plane occluder
        {
			auto entity = ecs->CreateEntity();
			auto& t = ecs->CreateComponent<Transform>(entity);
			t.position = { 0.0f, -400.0f };
            clvr::Sprite s;
            s.position = t.position;
			s.size = { 16000.0f, 200.0f };
            s.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            s.uvRect = renderer.GetAtlasRegion(L"../CloverRenderer/assets/textures/white.jpg").uvRect;
            s.layer = 99;
            s.isOccluder = true;
            ecs->CreateComponent<SpriteComponent>(entity, s);
        }
    }

    {
		auto lightEntity = ecs->CreateEntity();
		auto& transform = ecs->CreateComponent<Transform>(lightEntity);
		auto& light = ecs->CreateComponent<Light>(lightEntity);
		light.color = { 1.0f, 1.0f, 1.0f };
		light.direction = { 0.0f, 0.0f, -1.0f };
		light.intensity = 0.1f;
		light.type = 0; // directional light
    }

    {
        // ten varied point lights randomly positioned across the scene
        const int count = 10;
        for (int i = 0; i < count; ++i)
        {
            float t = (float) i / count;

            auto lightEntity = ecs->CreateEntity();
            ecs->CreateComponent<Transform>(lightEntity);
            auto& light = ecs->CreateComponent<Light>(lightEntity);
            ecs->CreateComponent<MovingLight>(lightEntity);

            // deterministic pseudo-random base position, seeded by index
            // (kept in sync with the wander logic in Update())
            float seedX = sinf((float) i * 12.9898f) * 43758.5453f;
            float seedY = sinf((float) i * 78.233f) * 43758.5453f;
            float baseX = fmodf(seedX, 1.0f) * 800.0f - 400.0f;
            float baseY = fmodf(seedY, 1.0f) * 400.0f - 200.0f;

            light.direction = { baseX, baseY, 0.0f };

            float angle = t * 2.0f * 3.1415927f;
            light.color = {
                0.5f + 0.5f * cosf(angle),
                0.5f + 0.5f * cosf(angle + 2.094f),
                0.5f + 0.5f * cosf(angle + 4.188f)
            };

            light.intensity = 1000.0f + t * 2000.0f;
            light.type = 1;
        }
    }

    /*{
        auto lightEntity = ecs->CreateEntity();
        ecs->CreateComponent<Transform>(lightEntity);
        auto& light = ecs->CreateComponent<Light>(lightEntity);

        light.direction = { 0.0f, 0.0f, 0.0f };
        light.color = { 1.0f, 1.0f, 1.0f };
        light.intensity = 5000.0f;
        light.type = 1;
    }*/
}

void Game::Update(float dt) {
    m_time += dt;

    auto ecs = Engine.GetECS();

    const float minI = 100.0f;
    const float maxI = 5000.0f;
    const float wanderRadius = 150.0f;
    const int   count = 10;
    {
        int index = 0;
        auto lightView = ecs->GetRegistry().view<Light, MovingLight>();
        for (auto [entity, light] : lightView.each())
        {
            if (index >= count)
                break;

            if (light.type != 1)
                continue;

            // same deterministic base position as SetupScene()
            float seedX = sinf((float) index * 12.9898f) * 43758.5453f;
            float seedY = sinf((float) index * 78.233f) * 43758.5453f;
            float baseX = fmodf(seedX, 1.0f) * 800.0f - 400.0f;
            float baseY = fmodf(seedY, 1.0f) * 400.0f - 200.0f;

            // wander around that base position, phase-offset per light
            float t = (float) index / count;
            float phase = m_time + t * 2.0f * 3.1415927f;
            float wanderX = cosf(phase * 0.7f) * wanderRadius;
            float wanderY = sinf(phase * 0.9f) * wanderRadius;

            light.direction = { baseX + wanderX, baseY + wanderY, 0.0f };

            float pulsePhase = m_time + t * 2.0f * 3.1415927f;
            light.intensity = minI + 0.5f * (sinf(pulsePhase) + 1.0f) * (maxI - minI);

            ++index;
        }
    }
    {
        auto spriteView = ecs->GetRegistry().view<SpriteComponent, Transform, MovingSprite>();

		for (auto [entity, sc, t] : spriteView.each())
		{
			//t.rotation += dt * 0.1f;
			// slowly move the sprite in an ellipsis
            t.position.x += (cosf(m_time) * dt * 300.0f);
			t.position.y += (sinf(m_time) * dt * 300.0f);
		}
    }

	auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
	auto input = Engine.GetInput();

    if (input->IsKeyDown('1'))
        renderer.SetActiveShader(L"default");
    else if (input->IsKeyDown('2'))
        renderer.SetActiveShader(L"grayscale");
    else if (input->IsKeyDown('3'))
        renderer.SetActiveShader(L"inverted");
    else if (input->IsKeyDown('4'))
        renderer.SetActiveShader(L"chromatic");
    else if (input->IsKeyDown('5'))
        renderer.SetActiveShader(L"wacky");

    if (input->IsKeyDown('R'))
    {
        renderer.ReloadShaders();
    }

    // move camera with arrow keys
    Camera& camera = renderer.GetCamera();
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
		camera.zoom += cameraSpeed * 0.01f;
		//if (camera.zoom > 5.0f)
			//camera.zoom = 5.0f;
    }
    if (input->IsKeyDown('S'))
    {
		camera.zoom -= cameraSpeed * 0.01f;
		if (camera.zoom < 0.1f)
			camera.zoom = 0.1f;
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
        camera.transform = Transform();
}

void Game::Render() {}