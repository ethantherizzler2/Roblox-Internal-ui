#include <Windows.h>
#include "../internal/render/render.h"
// - press insert to load the ui
namespace
{
    std::atomic<bool> RendererStarted{ false }; 
}

void MainThread()
{

    rbx::render::render();
    RendererStarted.store(true);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        std::thread(MainThread).detach();
    }
    return TRUE;
}
