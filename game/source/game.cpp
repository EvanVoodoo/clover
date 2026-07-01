#include "game.hpp"
#include "core/engine.hpp"
#include <core/components.hpp>

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

    renderer.LoadShader(L"default", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/color.ps.hlsl");
    renderer.LoadShader(L"grayscale", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/grayscale.ps.hlsl");
    renderer.LoadShader(L"inverted", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/inverted.ps.hlsl");
    renderer.LoadShader(L"chromatic", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/chromatic.ps.hlsl");
    renderer.LoadShader(L"wacky", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/wacky.ps.hlsl");

    renderer.LoadShader(L"light", L"../CloverRenderer/assets/shaders/post.vs.hlsl", L"../CloverRenderer/assets/shaders/light.ps.hlsl");
    renderer.LoadShader(L"composite", L"../CloverRenderer/assets/shaders/post.vs.hlsl", L"../CloverRenderer/assets/shaders/composite.ps.hlsl");
    renderer.LoadShader(L"passthrough", L"../CloverRenderer/assets/shaders/post.vs.hlsl", L"../CloverRenderer/assets/shaders/post.ps.hlsl");
    renderer.SetPostProcessShader(L"passthrough");
}

void Game::SetupScene()
{
    auto ecs = Engine.GetECS();
    auto& renderer = ecs->GetSystem<Renderer>();

    {
        const wchar_t* textures[] = {
            L"../CloverRenderer/assets/textures/shrew1.jpg",
            L"../CloverRenderer/assets/textures/shrew2.jpg",
            L"../CloverRenderer/assets/textures/hamper.jpeg",
        };
        for (auto* t : textures)
            renderer.AddTexture(t);
        renderer.BuildAtlas();   // built ONCE, after all textures added, before first frame

        auto centerEntity = ecs->CreateEntity();
		auto& t = ecs->CreateComponent<Transform>(centerEntity);
        t.position = { 0.0f, 0.0f };
        clvr::Sprite cs;
        cs.position = t.position;
        cs.size = { 600.0f, 600.0f };
        cs.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        cs.uvRect = renderer.GetAtlasRegion(L"../CloverRenderer/assets/textures/shrew1.jpg").uvRect;
        cs.layer = 0;
        ecs->CreateComponent<SpriteComponent>(centerEntity, cs);
    }

    {
		auto lightEntity = ecs->CreateEntity();
		auto& transform = ecs->CreateComponent<Transform>(lightEntity);
		auto& light = ecs->CreateComponent<Light>(lightEntity);
		light.color = { 0.8f, 0.3f, 0.1f };
		light.direction = { 0.0f, 0.0f, -1.0f };
		light.intensity = 0.2f;
		light.type = 0; // directional light
    }

    {
        // ten varied point lights arranged in a ring, differing color/position/intensity
        const int count = 10;
        for (int i = 0; i < count; ++i)
        {
            float t = (float)i / count;                 // 0..1 around the ring
            float angle = t * 2.0f * 3.1415927f;

            auto lightEntity = ecs->CreateEntity();
            ecs->CreateComponent<Transform>(lightEntity);
            auto& light = ecs->CreateComponent<Light>(lightEntity);

            // spread positions in a ring; direction.xy is overloaded as position for point lights
            float radius = 700.0f;
            light.direction = { cosf(angle) * radius, sinf(angle) * radius, 0.0f };

            // vary color by cycling hue-ish via the ring parameter
            light.color = {
                0.5f + 0.5f * cosf(angle),
                0.5f + 0.5f * cosf(angle + 2.094f),      // +120°
                0.5f + 0.5f * cosf(angle + 4.188f)       // +240°
            };

            light.intensity = 10000.0f + t * 20000.0f;   // 10000..30000, varied per light
            light.type = 1;                              // point light
        }
    }
}

void Game::Update(float dt) {
    m_time += dt;

    auto ecs = Engine.GetECS();

    const float minI = 1000.0f;
    const float maxI = 50000.0f;
    const float minR = 200.0f;      // closest to center
    const float maxR = 700.0f;      // farthest out
    const int   count = 10;

    int index = 0;
    auto lightView = ecs->GetRegistry().view<Light>();
    for (auto entity : lightView)
    {
        Light& light = lightView.get<Light>(entity);
        if (light.type != 1)
            continue;

        float base = (float)index / count;
        float angle = base * 2.0f * 3.1415927f;           // fixed slot on the ring

        // radius oscillates in/out, phase-offset per light so they move 1 by 1
        float phase = m_time + base * 2.0f * 3.1415927f;
        float r = minR + 0.5f * (sinf(phase) + 1.0f) * (maxR - minR);

        light.direction = { cosf(angle) * r, sinf(angle) * r, 0.0f };

        light.intensity = minI + 0.5f * (sinf(phase) + 1.0f) * (maxI - minI);

        ++index;
    }

    {
        auto spriteView = ecs->GetRegistry().view<SpriteComponent, Transform>();

		for (auto [entity, sc, t] : spriteView.each())
		{
			// rotate all sprites slowly
			t.rotation += dt * 0.1f;
			t.position.x += cosf(m_time) * dt * 300.0f;
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
        cameraSpeedMult = 10.0f;
    if (input->IsKeyDown(VK_CONTROL))
        cameraSpeedMult = 0.1f;
    float cameraSpeed = camera.speed * cameraSpeedMult * dt;

    if (input->IsKeyDown('Q'))
        camera.transform.rotation += (cameraSpeed * 0.1f * 2 * 3.1415927f) / 180.f;
    if (input->IsKeyDown('E'))
        camera.transform.rotation -= (cameraSpeed * 0.1f * 2 * 3.1415927f) / 180.f;

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