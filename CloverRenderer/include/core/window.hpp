#pragma once

#include <windows.h>

namespace clvr
{
	class Window
	{
	public:
		Window();
		Window(const Window&);
		~Window();

		bool Initialize(HINSTANCE hInstance, int nCmdShow, int width, int height);
		void Shutdown();

		HWND GetHWND() const;
		int GetWidth() const;
		int GetHeight() const;

	private:
		static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

		HWND m_hwnd;
		HINSTANCE m_hInstance;
		int m_width;
		int m_height;
	};
}