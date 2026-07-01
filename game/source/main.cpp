#include "framework.h"
#include "core/engine.hpp"
#include "game.hpp"

using namespace clvr;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    if (!Engine.Initialize(hInstance, nCmdShow))
        return 0;
    
    Engine.GetECS()->CreateSystem<Renderer>();
    Engine.GetECS()->CreateSystem<Game>();

    Engine.Run();
    Engine.Shutdown();
    return 0;
}