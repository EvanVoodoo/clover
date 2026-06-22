#include "framework.h"
#include "core/engine.hpp"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    clvr::Engine* engine = new clvr::Engine;

    if (!engine->Initialize(hInstance, nCmdShow))
        return 0;

    engine->Run();
    engine->Shutdown();

    delete engine;
    return 0;
}