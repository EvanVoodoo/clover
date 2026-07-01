#include "game.hpp"
#include "core/engine.hpp"
#include <core/components.hpp>

using namespace clvr;

Game::Game() {
    auto ecs = Engine.GetECS();
    auto& renderer = ecs->CreateSystem<Renderer>();
    SetupRenderer();

    const wchar_t* textures[] = {
        L"../CloverRenderer/assets/textures/shrew1.jpg",
        L"../CloverRenderer/assets/textures/shrew2.jpg",
        L"../CloverRenderer/assets/textures/hamper.jpeg",
    };
    for (auto* t : textures)
        renderer.AddTexture(t);
    renderer.BuildAtlas();   // built ONCE, after all textures added, before first frame

    auto centerEntity = ecs->CreateEntity();
    clvr::Sprite cs;
    cs.position = { 0.0f, 0.0f };
    cs.size = { 600.0f, 600.0f };
    cs.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    cs.uvRect = renderer.GetAtlasRegion(L"../CloverRenderer/assets/textures/shrew1.jpg").uvRect;
    cs.layer = 0;
	ecs->CreateComponent<SpriteComponent>(centerEntity, cs);
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

void Game::Update(float dt) {
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