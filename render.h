#pragma once
#include <windows.h>
#include <d3d11.h>
#include <../dependencies/imgui/imgui.h>
#include <../dependencies/imgui/backends/imgui_impl_dx11.h>
#include <../dependencies/imgui/backends/imgui_impl_win32.h>
#include "overlay.hpp"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

namespace rbx {
    class render {
    public:
        typedef HRESULT(WINAPI* present_fn)(IDXGISwapChain*, UINT, UINT);
        typedef HRESULT(WINAPI* resize_buffers_fn)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

        static void init();
        static bool imgui_init();

        static HRESULT WINAPI present_h(IDXGISwapChain* swapchain, UINT sync_intervals, UINT flags);
        static HRESULT WINAPI resize_buffers_h(IDXGISwapChain* swapchain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT flags);
        static LRESULT CALLBACK wnd_proc_h(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);

    private:
        static void render_hooks();

        static ID3D11Device* device;
        static ID3D11DeviceContext* device_context;
        static IDXGISwapChain* swap_chain;
        static ID3D11RenderTargetView* render_target_view;

        static present_fn original_present;
        static resize_buffers_fn original_resize_buffers;
        static WNDPROC original_wnd_proc;

        static HWND our_window;
        static bool is_init, draw;
        static float dpi_scale;
    };
}
