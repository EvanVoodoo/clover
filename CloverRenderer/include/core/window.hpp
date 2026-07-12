#pragma once

#include <windows.h>
#include "input.hpp"
#include <functional>

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
		void SetResizeCallback(std::function<void(int, int)> callback) { m_onResize = callback; }
		void SetActivateWindowCallback(std::function<void()> callback) { m_onActivateWindow = callback; }

	private:
		static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

		Input* m_input;

		HWND m_hwnd;
		HINSTANCE m_hInstance;
		int m_width;
		int m_height;
		std::function<void(int, int)> m_onResize;
		std::function<void()> m_onActivateWindow;
	};
}