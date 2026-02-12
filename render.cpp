#include "render.h"
#include <internal/utils.hpp>
#include <internal/globals.hpp>
#include <string>
#include <vector>
#include <iostream>

ID3D11Device* rbx::render::device = nullptr;
ID3D11DeviceContext* rbx::render::device_context = nullptr;
IDXGISwapChain* rbx::render::swap_chain = nullptr;
ID3D11RenderTargetView* rbx::render::render_target_view = nullptr;
rbx::render::present_fn rbx::render::original_present = nullptr;
rbx::render::resize_buffers_fn rbx::render::original_resize_buffers = nullptr;
WNDPROC rbx::render::original_wnd_proc = nullptr;
HWND rbx::render::our_window = nullptr;
bool rbx::render::is_init = false, rbx::render::draw = false;
float rbx::render::dpi_scale = 1.f;

bool IsRenderJob(uintptr_t Job) {
    __try {
        uintptr_t name_base = Job + Offsets::Scripts::Job_Name;
        const char* name_cstr = nullptr;

        name_cstr = *reinterpret_cast<const char**>(name_base);

        if (name_cstr && !IsBadReadPtr(name_cstr, 9)) {
            if (strcmp(name_cstr, "RenderJob") == 0) return true;
        }

        name_cstr = reinterpret_cast<const char*>(name_base);
        if (name_cstr && strcmp(name_cstr, "RenderJob") == 0) return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {} // watch for this part because it can cause crash i added safe check so doesnt crash!
    return false;
}

void rbx::render::render_hooks() {

    uintptr_t SchedulerPtr = *reinterpret_cast<uintptr_t*>(Offsets::RawScheduler);
    if (!SchedulerPtr) {
        Roblox::Print(3, "Failed to find RawScheduler"); // unmark these if u dont have print offset
        return;
    }

    uintptr_t JobsStart = *reinterpret_cast<uintptr_t*>(SchedulerPtr + Offsets::Scripts::JobStart);
    uintptr_t JobsEnd = *reinterpret_cast<uintptr_t*>(SchedulerPtr + Offsets::Scripts::JobStart + sizeof(void*));

    uintptr_t render_view = 0;

    for (uintptr_t i = JobsStart; i < JobsEnd; i += 0x10) {
        uintptr_t Job = *reinterpret_cast<uintptr_t*>(i);
        if (!Job) continue;

        if (IsRenderJob(Job)) {
            render_view = *reinterpret_cast<uintptr_t*>(Job + Offsets::render::renderview);
 
            break;
        }
    }

    if (!render_view) {
        Roblox::Print(3, "no renderjob sorry");
        return;
    }

    auto rbx_device = *reinterpret_cast<uintptr_t*>(render_view + Offsets::render::DeviceD3D11);
    if (!rbx_device) {
        Roblox::Print(3, "Failed to get device from render view");
        return;
    }

    swap_chain = *reinterpret_cast<IDXGISwapChain**>(rbx_device + Offsets::render::Swapchain);
    if (!swap_chain) {
        Roblox::Print(3, "Failed to get swap chain from device");
        return;
    }

    DXGI_SWAP_CHAIN_DESC desc;
    if (FAILED(swap_chain->GetDesc(&desc))) {
        Roblox::Print(3, "Failed to get swapchain description");
        return;
    }
    our_window = desc.OutputWindow;

    if (FAILED(swap_chain->GetDevice(__uuidof(ID3D11Device), (void**)&device))) {
        Roblox::Print(3, "Failed to get D3D11 device from swap chain");
        return;
    }

    device->GetImmediateContext(&device_context);
    
    // main part where ppl fuck up
    void** OriginalVTable = *reinterpret_cast<void***>(swap_chain);
    constexpr size_t VTableSize = 18;
    void** ShadowVTable = new void* [VTableSize];
    memcpy(ShadowVTable, OriginalVTable, sizeof(void*) * VTableSize);
    original_present = reinterpret_cast<present_fn>(OriginalVTable[8]);
    ShadowVTable[8] = reinterpret_cast<void*>(&present_h);

    original_resize_buffers = reinterpret_cast<resize_buffers_fn>(OriginalVTable[13]);
    ShadowVTable[13] = reinterpret_cast<void*>(&resize_buffers_h);

    DWORD old;
    if (VirtualProtect(swap_chain, sizeof(void*), PAGE_READWRITE, &old)) {
        *reinterpret_cast<void***>(swap_chain) = ShadowVTable;
        VirtualProtect(swap_chain, sizeof(void*), old, &old);
        Roblox::Print(1, "HOOKED");
    }
    else {
        Roblox::Print(3, "Failed to protect swap chain");
    }
    original_wnd_proc = (WNDPROC)SetWindowLongPtrW(our_window, GWLP_WNDPROC, (LONG_PTR)wnd_proc_h);

    if (original_wnd_proc) {
   // debug line if u want
    }
    else {
        Roblox::Print(3, "Failed to hook WndProc");
    }
}

HRESULT WINAPI rbx::render::present_h(IDXGISwapChain* swapchain, UINT sync, UINT flags) {
    if (!is_init) {
        is_init = imgui_init();
        if (is_init) Roblox::Print(3, "ImGui initialized");
    }

    if (!render_target_view && device) {
        ID3D11Texture2D* back_buffer = nullptr;
        if (SUCCEEDED(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer))) {
            device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
            back_buffer->Release();
        }
    }

    if (is_init && render_target_view && device_context) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (draw) c_gui::draw(&draw);

        ImGui::Render();
        device_context->OMSetRenderTargets(1, &render_target_view, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return original_present(swapchain, sync, flags);
}

HRESULT WINAPI rbx::render::resize_buffers_h(IDXGISwapChain* swapchain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT flags) {
    if (render_target_view) {
        render_target_view->Release();
        render_target_view = nullptr;
    }

    return original_resize_buffers(swapchain, buffer_count, width, height, new_format, flags);
}

LRESULT CALLBACK rbx::render::wnd_proc_h(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_KEYDOWN) {
        if (wparam == VK_INSERT || wparam == VK_RSHIFT || wparam == VK_HOME)
            draw = !draw;
    }
    else if (msg == WM_DPICHANGED) {
        dpi_scale = LOWORD(wparam) / 96.f;
    }

    extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (draw && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wparam, lparam))
        return true;

    if (draw) {
        switch (msg) {
        case WM_MOUSEWHEEL:
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_KEYUP:
        case WM_CHAR:
            return true;
        }
    }

    return CallWindowProc(original_wnd_proc, hWnd, msg, wparam, lparam);
}

bool rbx::render::imgui_init() {
    if (!our_window || !device || !device_context) return false;
    ImGui::CreateContext();
    if (!ImGui_ImplWin32_Init(our_window)) return false;
    if (!ImGui_ImplDX11_Init(device, device_context)) return false;
    return true;
}

void rbx::render::init() {
    render_hooks();
}
