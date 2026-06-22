#include "core/window.hpp"

using namespace clvr;

Window::Window() : m_hwnd(nullptr), m_hInstance(nullptr), m_width(0), m_height(0)
{
}

Window::Window(const Window&)
{
}

Window::~Window()
{
}

bool Window::Initialize(HINSTANCE hInstance, int nCmdShow, int width, int height)
{
	m_hInstance = hInstance;
	m_width = width;
	m_height = height;

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

	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
}