#include "core/window.hpp"
#include "core/imgui_layer.hpp"
#include <windowsx.h>

using namespace clvr;

Window::Window() : m_hwnd(nullptr), m_hInstance(nullptr), m_width(0), m_height(0)
{
	m_input = nullptr;
}

bool Window::Initialize(HINSTANCE hInstance, int nCmdShow, int width, int height, Input* input)
{
	m_hInstance = hInstance;
	m_width = width;
	m_height = height;

	m_input = input;

	// Register window class
	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = L"CloverWindow";

	if (!RegisterClassExW(&wcex))
		return false;

	// Create window
	m_hwnd = CreateWindowW(L"CloverWindow", L"Clover",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, width, height,
		nullptr, nullptr, hInstance, this);

	if (!m_hwnd)
		return false;

	ShowWindow(m_hwnd, nCmdShow);
	UpdateWindow(m_hwnd);

	return true;
}

void Window::Shutdown()
{
	if (m_hwnd)
	{
		DestroyWindow(m_hwnd);
		m_hwnd = nullptr;
	}

	UnregisterClassW(L"CloverWindow", m_hInstance);
}

HWND Window::GetHWND() const { return m_hwnd; }
int Window::GetWidth() const { return m_width; }
int Window::GetHeight() const { return m_height; }

void Window::SetMousePosition(Window* window, LPARAM lParam)
{
	int x = GET_X_LPARAM(lParam);
	int y = GET_Y_LPARAM(lParam);
	window->m_input->SetMousePosition(x, y);
}

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// Retrieve the Window instance pointer stored during CreateWindowW
	Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if (message == WM_NCCREATE)
	{
		CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	if (ImGuiLayer::WndProcHandler(hwnd, message, wParam, lParam))
		return true;

	switch (message)
	{
		/* Keyboard events */
		case WM_KEYDOWN:
			window->m_input->KeyDown(static_cast<unsigned int>(wParam));
			return 0;
		case WM_KEYUP:
			window->m_input->KeyUp(static_cast<unsigned int>(wParam));
			return 0;
		/* Mouse button events */
		case WM_MOUSEMOVE:
			SetMousePosition(window, lParam);
			return 0;
		case WM_LBUTTONDOWN:
			SetMousePosition(window, lParam);
			window->m_input->KeyDown(VK_LBUTTON);
			SetCapture(hwnd);
			return 0;
		case WM_LBUTTONUP:
			SetMousePosition(window, lParam);
			window->m_input->KeyUp(VK_LBUTTON);
			ReleaseCapture();
			return 0;
		case WM_RBUTTONDOWN:
			SetMousePosition(window, lParam);
			window->m_input->KeyDown(VK_RBUTTON);
			SetCapture(hwnd);
			return 0;
		case WM_RBUTTONUP:
			SetMousePosition(window, lParam);
			window->m_input->KeyUp(VK_RBUTTON);
			ReleaseCapture();
			return 0;
		case WM_MBUTTONDOWN:
			SetMousePosition(window, lParam);
			window->m_input->KeyDown(VK_MBUTTON);
			SetCapture(hwnd);
			return 0;
		case WM_MBUTTONUP:
			SetMousePosition(window, lParam);
			window->m_input->KeyUp(VK_MBUTTON);
			ReleaseCapture();
			return 0;
		/* Window events */
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_SIZE:
		{
			if (window) // guard against messages arriving before GWLP_USERDATA is set
			{
				window->m_width = LOWORD(lParam);
				window->m_height = HIWORD(lParam);

				if (window->m_onResize)
					window->m_onResize(window->m_width, window->m_height);
			}
			return 0;
		}
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_INACTIVE)
			{
				// Window is being deactivated
				// Handle deactivation logic here if needed
			}
			else
			{
				// Window is being activated
				// Handle activation logic here if needed
				if (window && window->m_onActivateWindow)
					window->m_onActivateWindow();
			}
			return 0;
		}
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
}