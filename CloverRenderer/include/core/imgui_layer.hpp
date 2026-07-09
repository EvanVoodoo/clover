#pragma once
#include <windows.h>  
#include "imgui_impl_dx11.h"

namespace clvr
{
    class ImGuiLayer
    {
    public:
        void Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
        void Shutdown();

        void BeginFrame();   // ImGui_ImplDX11_NewFrame, Win32_NewFrame, ImGui::NewFrame
        void EndFrame();     // ImGui::Render + ImGui_ImplDX11_RenderDrawData

        // Called from your existing WndProc
        static LRESULT WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    };
}