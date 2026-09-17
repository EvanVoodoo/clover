#include "core/input.hpp"
#include <cstring>

using namespace clvr;

Input::Input()
{
	for (int i = 0; i < 256; ++i)
	{
		m_keys[i] = false;
		m_lastKeys[i] = false;
	}
	m_mousePos[0] = 0;
	m_mousePos[1] = 0;
	m_lastMousePos[0] = 0;
	m_lastMousePos[1] = 0;
}

Input::Input(const Input& other)
{
	for (int i = 0; i < 256; ++i)
	{
		m_keys[i] = other.m_keys[i];
		m_lastKeys[i] = other.m_lastKeys[i];
	}
	m_mousePos[0] = other.m_mousePos[0];
	m_mousePos[1] = other.m_mousePos[1];
	m_lastMousePos[0] = other.m_lastMousePos[0];
	m_lastMousePos[1] = other.m_lastMousePos[1];
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

void Input::SetMousePosition(int x, int y)
{
	// Set the mouse position.
	m_mousePos[0] = x;
	m_mousePos[1] = y;
}


bool Input::IsKeyDown(unsigned int key) const
{
	// Return what state the key is in (pressed/not pressed).
	return m_keys[key];
}

bool Input::IsMouseButtonDown(unsigned int button) const
{
	return IsKeyDown(button);
}

bool Input::WasKeyJustReleased(unsigned int key) const
{
	return m_lastKeys[key] && !m_keys[key]; // was down last frame, not down now
}

bool Input::WasKeyJustPressed(unsigned int key) const
{
	return !m_lastKeys[key] && m_keys[key]; // wasn't down last frame, is down now
}

bool Input::IsMouseMoving() const
{
	return m_mousePos[0] != m_lastMousePos[0] ||
		m_mousePos[1] != m_lastMousePos[1];
}

bool Input::WasMouseButtonJustReleased(unsigned int button) const
{
	return WasKeyJustReleased(button);
}

bool Input::WasMouseButtonJustPressed(unsigned int button) const
{
	return WasKeyJustPressed(button);
}

void Input::EndFrame()
{
	memcpy(m_lastKeys, m_keys, sizeof(m_keys));
	m_lastMousePos[0] = m_mousePos[0];
	m_lastMousePos[1] = m_mousePos[1];
}