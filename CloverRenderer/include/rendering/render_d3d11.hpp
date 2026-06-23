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
#include "shader_manager.hpp"
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

		bool Initialize(int, int, bool, HWND, bool, float, float);
		bool InitializeFullscreenQuad();
		void Shutdown();

		void BeginScene(float, float, float, float);
		void EndScene();
		void DrawSprite(const Sprite& sprite);
		void SetActiveShader(const std::wstring& name) { m_shaderManager->SetActiveShader(name); }
		void SetPostProcessShader(const std::wstring& name) { m_shaderManager->SetPostProcessShader(name); }

		bool LoadShader(const std::wstring& name, const wchar_t* vsFilename, const wchar_t* psFilename)
		{
			return m_shaderManager->LoadShader(name, vsFilename, psFilename);
		}

		bool ReloadShaders() { return m_shaderManager->ReloadAll(); }

		ID3D11ShaderResourceView* LoadTexture(const wchar_t* filename);
		AtlasRegion GetAtlasRegion(const wchar_t* filename) { return m_textureAtlas->GetRegion(filename); }

		ID3D11Device* GetDevice();
		ID3D11DeviceContext* GetDeviceContext();

		void GetProjectionMatrix(XMMATRIX&);
		void GetWorldMatrix(XMMATRIX&);

		void GetVideoCardInfo(char*, int&);

		void SetBackBufferRenderTarget();
		void ResetViewport();

	private:
		bool m_vsyncEnabled;
		int m_videoCardMemory;
		char m_videoCardDescription[128];
		IDXGISwapChain* m_swapChain;
		ID3D11Device* m_device;
		ID3D11DeviceContext* m_deviceContext;
		ID3D11RenderTargetView* m_renderTargetView;
		
		Framebuffer* m_framebuffer;

		ID3D11Buffer* m_fullscreenQuadVB;
		ID3D11Buffer* m_fullscreenQuadIB;

		ID3D11RasterizerState* m_rasterState;
		XMMATRIX m_projectionMatrix;
		XMMATRIX m_worldMatrix;
		D3D11_VIEWPORT m_viewport;

		ShaderManager* m_shaderManager;
		SpriteBatcher* m_spriteBatcher;
		ID3D11SamplerState* m_samplerState;
		TextureAtlas* m_textureAtlas;
	};
}