#include "core/imgui_layer.hpp"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

using namespace clvr;

void ImGuiLayer::Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    // scale style + fonts to match monitor DPI
    float dpiScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY));
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(dpiScale);
    style.FontScaleDpi = dpiScale;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, context);
}

void ImGuiLayer::Shutdown()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::BeginFrame()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::EndFrame()
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT ImGuiLayer::WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() == nullptr)
        return 0;

    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
}
