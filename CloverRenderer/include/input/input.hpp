#pragma once

namespace clvr
{
	namespace INPUT 
	{
		// Mouse button enumeration - corresponds to the Windows API mouse button flags
		enum MOUSE_BUTTON
		{
			LEFT = 0x01,
			RIGHT = 0x02,
			MIDDLE = 0x04
		};
	}

	class Input
	{
	public:
		Input();
		Input(const Input&);
		~Input();
		bool Initialize();
		void Shutdown();
		void EndFrame();

		void KeyDown(unsigned int);
		void KeyUp(unsigned int);
		void SetMousePosition(int x, int y);

		bool IsKeyDown(unsigned int) const;
		bool IsMouseButtonDown(unsigned int button) const;

		bool WasKeyJustReleased(unsigned int key) const;
		bool WasKeyJustPressed(unsigned int key) const;
		bool WasMouseButtonJustReleased(unsigned int button) const;
		bool WasMouseButtonJustPressed(unsigned int button) const;

		bool IsMouseMoving() const;

		const int* GetMousePosition() const { return m_mousePos; }
		int GetMouseX() const { return m_mousePos[0]; }
		int GetMouseY() const { return m_mousePos[1]; }
	private:
		bool m_keys[256] = {};
		bool m_lastKeys[256] = {};
		int m_mousePos[2] = { 0, 0 };
		int m_lastMousePos[2] = { 0, 0 };
	};
}