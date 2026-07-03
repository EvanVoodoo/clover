#pragma once

// Based on the tutorial from http://www.rastertek.com/dx11tut03.html

#include "core/ecs.hpp"
#include "rendering/render_d3d11.hpp"	

const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = false;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;
const int MAX_LIGHTS = 16;

namespace clvr
{

	class Renderer : public System
	{
	public:
		Renderer();
		Renderer(const Renderer&);
		~Renderer();

		bool Initialize(int, int, HWND);
		void Shutdown();

		void Update(float dt);
		void Render() {}

		bool Frame(float dt);
		void DrawSprite(const Sprite& sprite, const Transform& transform);
		void SetActiveShader(const std::wstring& name) { m_DX2D->SetActiveShader(name); }
		void SetPostProcessShader(const std::wstring& name) { m_DX2D->SetPostProcessShader(name); }
		bool LoadShader(const std::wstring& name, const wchar_t* vsFilename, const wchar_t* psFilename)
		{
			return m_DX2D->LoadShader(name, vsFilename, psFilename);
		}
		bool ReloadShaders() { return m_DX2D->ReloadShaders(); }
		Camera& GetCamera() { return m_DX2D->GetCamera(); }

		void UpdateLights();

		int AddTexture(const wchar_t* filename);
		bool BuildAtlas();
		AtlasRegion GetAtlasRegion(const wchar_t* filename); // if not already exposed

		int priority = 0;
		std::string title = "Renderer";

	private:
		bool Render(float dt);

	private:
		DirectX2D* m_DX2D;
	};
}