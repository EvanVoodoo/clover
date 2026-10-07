#pragma once

// Based on the tutorial from http://www.rastertek.com/dx11tut03.html

#include "core/ecs.hpp"
#include <memory>
#include <string>
#include <vector>
#include <core/transform.hpp>
#include "render_components.hpp"
#include "texture.hpp"
#include "texture_atlas.hpp"

const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = false;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;

namespace clvr
{
	class DirectX2D;

	// abstract interface for the renderer, to allow for different rendering backends in the future
	class IRenderer
	{
	public:
		virtual void* GetNativeDeviceHandle() = 0; // opaque on purpose
		virtual ~IRenderer() = default;
	};

	class Renderer : public System, public IRenderer
	{
	public:
		Renderer();
		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		~Renderer();

		bool Initialize(int, int);
		void Shutdown();

		void Update(float dt);
		void Render();
		void Inspect(float dt);
		json Save();
		void Load(const json& j);

		void* GetNativeDeviceHandle() override;

		void DrawSprite(const Sprite& sprite, const Transform& transform, const SpriteLayer& layer, const Transform& cameraTransform);
		void DrawUnbatchedSprite(const Sprite& sprite, const Transform& transform, const SpriteLayer& layer);
		void SetActiveShader(const std::wstring& name);
		void SetPostProcessShader(const std::wstring& name);
		bool LoadShader(const std::string& name, const std::string& vsFilename, const std::string& psFilename);
		bool ReloadShaders();
		Camera& GetActiveCamera();

		bool SetFullscreen(bool fullscreen);
		bool IsFullscreen() const;

		void UpdateLights();

		int AddTexture(const std::string filename);
		std::shared_ptr<Texture> LoadTexture(std::string filename);
		bool BuildAtlas();
		AtlasRegion GetAtlasRegion(const std::string filename);
		TextureAtlas* GetTextureAtlas();

		SpriteLayer* CreateSpriteLayer(const unsigned int id, float parallaxFactor = 1.0f, const std::string& layerName = "");
		SpriteLayer* FindSpriteLayer(unsigned int id);
		SpriteLayer* FindOrCreateSpriteLayer(unsigned int id);
		std::vector<SpriteLayer*>& GetSpriteLayers() { return m_spriteLayers; }

	private:
		void EditorWindowControls(float dt);

	private:
		DirectX2D* m_DX2D;
		bool m_fullscreenMemory = false;
		XMFLOAT2 m_scaledMouseNorm = { 0.0f, 0.0f };
		std::vector<SpriteLayer*> m_spriteLayers; // store sprite layers for sorting

		bool m_gameWindowFocused = false;
	};
}