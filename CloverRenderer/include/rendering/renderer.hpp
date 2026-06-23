#pragma once

// Based on the tutorial from http://www.rastertek.com/dx11tut03.html

#include "rendering/render_d3d11.hpp"	

const bool FULL_SCREEN = true;
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;

namespace clvr
{

	class Renderer
	{
	public:
		Renderer();
		Renderer(const Renderer&);
		~Renderer();

		bool Initialize(int, int, HWND);
		void Shutdown();
		bool Frame(float dt);
		void DrawSprite(const Sprite& sprite);

	private:
		bool Render(float dt);

	private:
		DirectX2D* m_DX2D;
	};
}