#include "rendering/framebuffer.hpp"

using namespace clvr;

Framebuffer::Framebuffer()
{
	m_texture = nullptr;
	m_renderTargetView = nullptr;
	m_srv = nullptr;
}

Framebuffer::~Framebuffer()
{
}

bool Framebuffer::Initialize(ID3D11Device* device, int width, int height)
{
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	HRESULT result = device->CreateTexture2D(&textureDesc, nullptr, &m_texture);
	if (FAILED(result))
		return false;

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = textureDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	result = device->CreateRenderTargetView(m_texture, &rtvDesc, &m_renderTargetView);
	if (FAILED(result))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	result = device->CreateShaderResourceView(m_texture, &srvDesc, &m_srv);
	if (FAILED(result))
		return false;

	return true;
}

void Framebuffer::Shutdown()
{
	if (m_srv)
	{
		m_srv->Release();
		m_srv = nullptr;
	}
	if (m_renderTargetView)
	{
		m_renderTargetView->Release();
		m_renderTargetView = nullptr;
	}
	if (m_texture)
	{
		m_texture->Release();
		m_texture = nullptr;
	}
}

void Framebuffer::Bind(ID3D11DeviceContext* deviceContext)
{
	deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
}

ID3D11RenderTargetView* Framebuffer::GetRTV() const
{
	return m_renderTargetView;
}

ID3D11ShaderResourceView* Framebuffer::GetSRV() const
{
	return m_srv;
}