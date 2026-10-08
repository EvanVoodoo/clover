#include "framework.h"
#include "core/engine.hpp"
#include "game.hpp"
#include "rendering/renderer.hpp"
#include <core/scene.hpp>
#include <input/input_handler.hpp>

using namespace clvr;

using Dir = ResourceManager::Directory;

void SetupRenderer()
{
	auto ecs = Engine.GetECS();
	auto& renderer = ecs->GetSystem<Renderer>();
	ResourceManager* rMngr = Engine.GetResourceManager();

	renderer.Initialize(SCREEN_WIDTH, SCREEN_HEIGHT);

	renderer.LoadShader("grayscale",
		rMngr->GetPath(Dir::SharedAssets, "shaders/color.vs.hlsl"),
		rMngr->GetPath(Dir::SharedAssets, "shaders/grayscale.ps.hlsl"));
	renderer.LoadShader("inverted",
		rMngr->GetPath(Dir::SharedAssets, "shaders/color.vs.hlsl"),
		rMngr->GetPath(Dir::SharedAssets, "shaders/inverted.ps.hlsl"));
	renderer.LoadShader("chromatic",
		rMngr->GetPath(Dir::SharedAssets, "shaders/color.vs.hlsl"),
		rMngr->GetPath(Dir::SharedAssets, "shaders/chromatic.ps.hlsl"));
	renderer.LoadShader("wacky",
		rMngr->GetPath(Dir::SharedAssets, "shaders/color.vs.hlsl"),
		rMngr->GetPath(Dir::SharedAssets, "shaders/wacky.ps.hlsl"));
	renderer.LoadShader("crt",
		rMngr->GetPath(Dir::SharedAssets, "shaders/post.vs.hlsl"),
		rMngr->GetPath(Dir::SharedAssets, "shaders/crt.ps.hlsl"));
	//renderer.SetPostProcessShader(L"crt");

	renderer.CreateSpriteLayer(3, 0.2f, "Further Background Layer");
	renderer.CreateSpriteLayer(2, 0.5f, "Background Layer");
	renderer.CreateSpriteLayer(1, 1.0f, "Sprite Layer");
	renderer.CreateSpriteLayer(0, 1.0f, "Default Layer");
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (!Engine.Initialize(hInstance, nCmdShow))
		return 0;

	auto ecs = Engine.GetECS();

	ecs->CreateSystem<Renderer>();
	SetupRenderer();

	ecs->CreateSystem<InputHandler>();
	ecs->CreateSystem<SceneManager>();
	ecs->CreateSystem<Game>();

	Engine.Run();
	Engine.Shutdown();
	return 0;
}