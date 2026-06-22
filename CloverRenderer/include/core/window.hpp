#pragma once

#include <windows.h>
#include "input.hpp"

namespace clvr
{
	class Window
	{
	public:
		Window();
		Window(const Window&);
		~Window();

		bool Initialize(HINSTANCE hInstance, int nCmdShow, int width, int height, Input* input);
		void Shutdown();

		HWND GetHWND() const;
		int GetWidth() const;
		int GetHeight() const;

	private:
		static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

		Input* m_input;

		HWND m_hwnd;
		HINSTANCE m_hInstance;
		int m_width;
		int m_height;
	};
}