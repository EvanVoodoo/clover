#pragma once

// Based on the tutorial from http://www.rastertek.com/dx11tut03.html

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <d3d11.h>
#include <directxmath.h>
#include "shader.hpp"
#include "sprite_batcher.hpp"
#include "texture_atlas.hpp"

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
		void Shutdown();

		void BeginScene(float, float, float, float);
		void EndScene();
		void DrawSprite(const Sprite& sprite);

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

		/* Not needed for a 2D renderer
		ID3D11Texture2D* m_depthStencilBuffer;
		ID3D11DepthStencilState* m_depthStencilState;
		ID3D11DepthStencilView* m_depthStencilView;*/

		ID3D11RasterizerState* m_rasterState;
		XMMATRIX m_projectionMatrix;
		XMMATRIX m_worldMatrix;
		D3D11_VIEWPORT m_viewport;

		ID3D11Buffer* m_vertexBuffer;
		ID3D11InputLayout* m_inputLayout;
		Shader* m_shader;
		SpriteBatcher* m_spriteBatcher;
		ID3D11SamplerState* m_samplerState;
		TextureAtlas* m_textureAtlas;
	};
}