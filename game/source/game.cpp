#include "game.hpp"
#include "core/engine.hpp"
#include "core/components.hpp" 
#include "rendering/renderer.hpp"

using namespace clvr;

Game::Game() {
    priority = 10;
    title = "GameSystem";

    SetupRenderer();
    SetupScene();
}

void Game::SetupRenderer()
{
    auto ecs = Engine.GetECS();
    auto& renderer = ecs->GetSystem<Renderer>();
    renderer.Initialize(SCREEN_WIDTH, SCREEN_HEIGHT, Engine.GetWindow()->GetHWND());

    renderer.LoadShader(L"grayscale", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/grayscale.ps.hlsl");
    renderer.LoadShader(L"inverted", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/inverted.ps.hlsl");
    renderer.LoadShader(L"chromatic", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/chromatic.ps.hlsl");
    renderer.LoadShader(L"wacky", L"../CloverRenderer/assets/shaders/color.vs.hlsl", L"../CloverRenderer/assets/shaders/wacky.ps.hlsl");
    renderer.LoadShader(L"crt", L"../CloverRenderer/assets/shaders/post.vs.hlsl", L"../CloverRenderer/assets/shaders/crt.ps.hlsl");
    //renderer.SetPostProcessShader(L"crt");

	renderer.CreateSpriteLayer(3, 0.2f, "Further Background Layer");
	renderer.CreateSpriteLayer(2, 0.5f, "Background Layer");
	renderer.CreateSpriteLayer(1, 1.0f, "Sprite Layer");
	renderer.CreateSpriteLayer(0, 1.0f, "Default Layer");
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

    {
        auto centerEntity = ecs->CreateEntity();
        auto& t = ecs->CreateComponent<Transform>(centerEntity);
        t.position = { 0.0f, -300.0f };
        clvr::Sprite cs;
        cs.position = t.position;
        cs.size = { 200.0f, 200.0f };
        cs.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        cs.uvRect = renderer.GetAtlasRegion(L"../CloverRenderer/assets/textures/saturn.png").uvRect;
		SpriteLayer* layer = renderer.FindOrCreateSpriteLayer(1);
        cs.layer = renderer.FindOrCreateSpriteLayer(1);
        cs.isOccluder = false;
        ecs->CreateComponent<SpriteComponent>(centerEntity, cs);
        //ecs->CreateComponent<MovingSprite>(centerEntity);
    }

    {
		// create wall of occluder sprites randomly around the scene
        for (int i = 0; i < 32; ++i)
        {
            auto entity = ecs->CreateEntity();
            auto& t = ecs->CreateComponent<Transform>(entity);
            t.position = { (float) (rand() % 3200 - 1600), (float) (rand() % 1600 - 800) };
            t.rotation = (float) (rand() % 360) * 3.1415927f / 180.0f;
            clvr::Sprite s;
            s.position = t.position;
            s.size = { (float) (rand() % 200 + 100), (float) (rand() % 100 + 50) };
            s.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            s.uvRect = renderer.GetAtlasRegion(L"../CloverRenderer/assets/textures/white.jpg").uvRect;
            s.layer = renderer.FindOrCreateSpriteLayer(0);
            s.isOccluder = true;
            ecs->CreateComponent<SpriteComponent>(entity, s);
        }
        for (int i = 0; i < 32; ++i)
        {
            auto entity = ecs->CreateEntity();
            auto& t = ecs->CreateComponent<Transform>(entity);
            t.position = { (float) (rand() % 3200 - 1600), (float) (rand() % 1600 - 800) };
            t.rotation = (float) (rand() % 360) * 3.1415927f / 180.0f;
            clvr::Sprite s;
            s.position = t.position;
            s.size = { (float) (rand() % 400 + 200), (float) (rand() % 200 + 100) };
            s.color = { 0.5f, 0.0f, 1.0f, 1.0f };
            s.uvRect = renderer.GetAtlasRegion(L"../CloverRenderer/assets/textures/white.jpg").uvRect;
			SpriteLayer* layer = renderer.FindOrCreateSpriteLayer(2);
            s.layer = layer;
            s.isOccluder = false;
            ecs->CreateComponent<SpriteComponent>(entity, s);
        }
        {
            auto entity = ecs->CreateEntity();
            auto& t = ecs->CreateComponent<Transform>(entity);
            t.position = { 0, 0 };
            clvr::Sprite s;
            s.position = t.position;
            s.size = { 3000, 3000 };
            s.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            s.uvRect = renderer.GetAtlasRegion(L"../CloverRenderer/assets/textures/shrew1.jpg").uvRect;
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
            clvr::Sprite s;
            s.position = t.position;
			s.size = { 16000.0f, 64.0f };
            s.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            s.uvRect = renderer.GetAtlasRegion(L"../CloverRenderer/assets/textures/white.jpg").uvRect;
            s.layer = renderer.FindOrCreateSpriteLayer(0);
            s.isOccluder = true;
            ecs->CreateComponent<SpriteComponent>(entity, s);
        }
    }

	{ // create a single directional light pointing downwards
		auto lightEntity = ecs->CreateEntity();
		auto& transform = ecs->CreateComponent<Transform>(lightEntity);
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

    //const float minI = 10.0f;
    //const float maxI = 50000.0f;
    //const float wanderRadius = 150.0f;
    //{
    //    int index = 0;
    //    auto lightView = ecs->GetRegistry().view<Light, Transform, MovingLight>();
    //    const int count = static_cast<int>(lightView.size_hint());
    //    for (auto [entity, light, transform] : lightView.each())
    //    {
    //        if (light.type != 1)
    //            continue;

    //        // same deterministic base position as SetupScene()
    //        float seedX = sinf((float) index * 12.9898f) * 43758.5453f;
    //        float seedY = sinf((float) index * 78.233f) * 43758.5453f;
    //        float baseX = fmodf(seedX, 1.0f) * 1600.0f;
    //        float baseY = fmodf(seedY, 1.0f) * 800.0f;

    //        // wander around that base position, phase-offset per light
    //        float t = (float) index * index * 2.17f / count;
    //        float phase = m_time + t * 2.0f * 3.1415927f;
    //        float wanderX = cosf(phase * 0.7f) * wanderRadius;
    //        float wanderY = sinf(phase * 0.9f) * wanderRadius;

    //        light.direction = { baseX + wanderX, baseY + wanderY, 0.0f };
    //        transform.position = { light.direction.x, light.direction.y }; // update the Transform position to match the light's direction

    //        float pulsePhase = m_time + t * 2.0f * 3.1415927f;
    //        light.intensity = minI + 0.5f * (sinf(pulsePhase) + 1.0f) * (maxI - minI);

    //        ++index;
    //    }
    //}

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

    {
        auto spriteView = ecs->GetRegistry().view<SpriteComponent, Transform, MovingSprite>();

        for (auto [entity, sc, t] : spriteView.each())
        {
			t.scale = { 1.0f + 0.5f * sinf(m_time), 1.0f + 0.5f * cosf(m_time) };
            t.rotation += (sinf(m_time) * dt * 2.0f);
            // slowly move the sprite in an ellipsis
            t.position.x += (cosf(m_time) * dt * 300.0f);
            t.position.y += (sinf(m_time) * dt * 300.0f);
			//camera.transform = t; // follow the moving sprite with the camera
        }
    }

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

void Game::Render() {}

void Game::Inspect(float dt) {
	auto& renderer = Engine.GetECS()->GetSystem<Renderer>();

	ImGui::Begin("Game System");

    ImGui::Text("Frame time: %.3f s", dt);

    // example: iterate lights and show their properties
    auto view = Engine.GetECS()->GetRegistry().view<Transform, Light>();
    int i = 0;
    for (auto [entity, transform, light] : view.each())
    {
        ImGui::PushID(i++);
        string label = "";
        if (light.type == 0.f) {
			label = "Directional Light";
            if (ImGui::TreeNode(label.c_str()))
            {
                float angle = atan2f(light.direction.y, light.direction.x);

                if (ImGui::SliderAngle("Direction", &angle, -180.0f, 180.0f))
                {
                    light.direction.x = cosf(angle);
                    light.direction.y = sinf(angle);
                }
                ImGui::ColorEdit3("Color", &light.color.x);
                ImGui::DragFloat("Intensity", &light.intensity, 0.01f, 0.0f, 10.0f);
                ImGui::TreePop();
            }
        }
        else if (light.type == 1.f) {
			label = "Point Light";
            if (ImGui::TreeNode(label.c_str()))
            {
                if (ImGui::DragFloat2("Position", &transform.position.x)) {
                    light.direction.x = transform.position.x;
                    light.direction.y = transform.position.y;
                }
                ImGui::ColorEdit3("Color", &light.color.x);
                ImGui::DragFloat("Intensity", &light.intensity);
                ImGui::TreePop();
            }
        }
        
        ImGui::PopID();
    }

	auto spriteView = Engine.GetECS()->GetRegistry().view<Transform, SpriteComponent>();
	for (auto [entity, transform, sprite] : spriteView.each())
	{
		ImGui::PushID(i++);
		string label = "Sprite " + std::to_string(i);
		if (ImGui::TreeNode(label.c_str()))
		{
			if (ImGui::DragFloat2("Position", &transform.position.x)) {
				sprite.sprite.position.x = transform.position.x;
				sprite.sprite.position.y = transform.position.y;
			}
            ImGui::DragFloat2("Size", &sprite.sprite.size.x, 1.0f, 0.0f);
            ImGui::DragFloat2("Scale", &transform.scale.x);
			if (ImGui::SliderAngle("Rotation", &transform.rotation, -180.0f, 180.0f)) {
				// rotation is in radians
			}
			ImGui::ColorEdit4("Color", &sprite.sprite.color.x);

            const auto& layers = renderer.GetSpriteLayers();
            SpriteLayer* currentLayer = renderer.FindSpriteLayer(sprite.sprite.layer->id);
            std::string currentLayerName = currentLayer ? currentLayer->layerName : "None";

            if (ImGui::BeginCombo("Layer", currentLayerName.c_str()))
            {
                for (SpriteLayer* layer : layers)
                {
                    bool isSelected = (layer == currentLayer);
                    if (ImGui::Selectable(layer->layerName.c_str(), isSelected))
                        sprite.sprite.layer = layer;

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

			ImGui::TreePop();
		}
		ImGui::PopID();
	}

	ImGui::End();
}