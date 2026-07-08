#pragma once

// Based on the tutorial from http://www.rastertek.com/dx11tut03.html

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <d3d11.h>
#include <directxmath.h>
#include "shader_manager.hpp"
#include "sprite_batcher.hpp"
#include "texture_atlas.hpp"
#include "framebuffer.hpp"

using namespace DirectX;

namespace clvr
{
	class DirectX2D
	{
	public:
		DirectX2D();
		DirectX2D(const DirectX2D&);
		~DirectX2D();

		bool Initialize(int, int, bool, HWND, bool);
		bool InitializeFullscreenQuad();
		void Shutdown();

		void BeginScene(float, float, float, float);
		void EndScene();
		void OcclusionRender();
		void DrawSprite(const Sprite& sprite, const Transform& transform);
		void SetActiveShader(const std::wstring& name) { m_shaderManager->SetActiveShader(name); }
		void SetPostProcessShader(const std::wstring& name) { m_shaderManager->SetPostProcessShader(name); }

		bool LoadShader(const std::wstring& name, const wchar_t* vsFilename, const wchar_t* psFilename)
		{
			return m_shaderManager->LoadShader(name, vsFilename, psFilename);
		}

		bool ReloadShaders() { return m_shaderManager->ReloadAll(); }

		int AddTexture(const wchar_t* filename);
		ID3D11ShaderResourceView* LoadTexture(const wchar_t* filename);

		AtlasRegion GetAtlasRegion(const wchar_t* filename) { return m_textureAtlas->GetRegion(filename); }
		bool BuildAtlas();

		ID3D11Device* GetDevice();
		ID3D11DeviceContext* GetDeviceContext();

		XMMATRIX GetProjectionMatrix();
		XMMATRIX GetWorldMatrix();
		XMMATRIX GetViewMatrix();
		Camera& GetCamera() { return m_camera; }

		void GetVideoCardInfo(char*, int&);

		void SetBackBufferRenderTarget();
		void ResetViewport();

		void UpdateLights(const BufferType::LightBufferType& data)
		{
			m_lightCb.Update(m_deviceContext, data);
			m_lights = data;
		}
		void UpdateWindowSize(float w, float h)
		{
			m_currentWindowSize = { w, h };
		}

	private:
		bool m_vsyncEnabled;
		int m_videoCardMemory;
		char m_videoCardDescription[128];
		float m_aspectRatio = 1920.0f / 1080.0f;
		IDXGISwapChain* m_swapChain;
		ID3D11Device* m_device;
		ID3D11DeviceContext* m_deviceContext;
		ID3D11RenderTargetView* m_renderTargetView;
		
		Framebuffer* m_framebuffer;
		Framebuffer* m_occluderMaskFramebuffer;
		Framebuffer* m_lightFramebuffer;
		Framebuffer* m_postFramebuffer;
		Framebuffer* m_finalFramebuffer;
		Framebuffer* m_occlusionFramebuffers[MAX_LIGHTS];
		Framebuffer* m_shadowMapSingleFb;

		ID3D11Buffer* m_fullscreenQuadVB;
		ID3D11Buffer* m_fullscreenQuadIB;

		ID3D11RasterizerState* m_rasterState;

		// main camera for 2D rendering
		Camera m_camera;
		D3D11_VIEWPORT m_viewport;

		XMFLOAT2 m_currentWindowSize;

		ConstantBuffer<BufferType::MVPBufferType> m_mvpCb;
		ConstantBuffer<BufferType::LightBufferType> m_lightCb;

		// lights for the scene, updated each frame, used for shadow mapping
		BufferType::LightBufferType m_lights;

		ShaderManager* m_shaderManager;
		SpriteBatcher* m_spriteBatcher;
		ID3D11SamplerState* m_pointSampler;
		ID3D11SamplerState* m_linearSampler;
		TextureAtlas* m_textureAtlas;
	};
}