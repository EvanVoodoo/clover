#include "core/input.hpp"

using namespace clvr;

Input::Input()
{
	for (int i = 0; i < 256; ++i)
	{
		m_keys[i] = false;
	}
}

Input::Input(const Input&)
{
}

Input::~Input()
{
}

bool Input::Initialize() {
	for (int i = 0; i < 256; ++i)
	{
		m_keys[i] = false;
	}
	return true;
}

void Input::Shutdown()
{

}

void Input::KeyDown(unsigned int input)
{
	// If a key is pressed then save that state in the key array.
	m_keys[input] = true;
	return;
}

void Input::KeyUp(unsigned int input)
{
	// If a key is released then clear that state in the key array.
	m_keys[input] = false;
	return;
}

bool Input::IsKeyDown(unsigned int key)
{
	// Return what state the key is in (pressed/not pressed).
	return m_keys[key];
}
