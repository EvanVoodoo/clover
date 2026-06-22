#pragma once

namespace clvr
{
	class Input
	{
	public:
		Input();
		Input(const Input&);
		~Input();
		bool Initialize();
		void Shutdown();

		void KeyDown(unsigned int);
		void KeyUp(unsigned int);

		bool IsKeyDown(unsigned int);
	private:
		bool m_keys[256];
	};
}