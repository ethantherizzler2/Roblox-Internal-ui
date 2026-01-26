
#pragma once
#include <Windows.h>
 // version-8b8c34790f814a73
#define REBASE(Address) (Address + reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)))

namespace Offsets
{
    const uintptr_t Print = REBASE(0x1737F90);

    namespace render {
        const uintptr_t renderview = 0x218; // most of the time the same
        const uintptr_t DeviceD3D11 = 0x8; // same
        const uintptr_t VisualEngine = 0x10; // same
        const uintptr_t Swapchain = 0xD0;  // same

    }
}

namespace Roblox
{
    inline auto Print = (uintptr_t(__fastcall*)(int, const char*, ...))Offsets::Print;
}
