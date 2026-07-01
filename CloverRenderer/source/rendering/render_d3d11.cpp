#include "rendering/render_d3d11.hpp"
#include <DirectXTex.h>

#pragma comment(lib, "DirectXTex.lib")

using namespace clvr;

DirectX2D::DirectX2D()
{
	m_swapChain = nullptr;
	m_device = nullptr;
	m_deviceContext = nullptr;
	m_renderTargetView = nullptr;
	/*m_depthStencilBuffer = nullptr;
	m_depthStencilState = nullptr;
	m_depthStencilView = nullptr;*/
	m_rasterState = nullptr;
	m_shaderManager = nullptr;
	m_spriteBatcher = nullptr;
	m_samplerState = nullptr;
	m_textureAtlas = nullptr;
	m_framebuffer = nullptr;
	m_lightFramebuffer = nullptr;
	m_finalFramebuffer = nullptr;
	m_fullscreenQuadIB = nullptr;
	m_fullscreenQuadVB = nullptr;
}

DirectX2D::DirectX2D(const DirectX2D& other)
{
	(void)other;
}

DirectX2D::~DirectX2D()
{
}

bool DirectX2D::Initialize(int screenWidth, int screenHeight, bool vsync, HWND hwnd, bool fullscreen, float screenDepth, float screenNear)
{
	HRESULT result;
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes, i, numerator, denominator;
	unsigned long long stringLength;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	int error;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Texture2D* backBufferPtr;
	//D3D11_TEXTURE2D_DESC depthBufferDesc;
	//D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	//D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_RASTERIZER_DESC rasterDesc;

	// Get the refresh rate of the monitor that matches the screen width and height provided.

	// Store the vsync setting.
	m_vsyncEnabled = vsync;

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(result))
	{
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	result = factory->EnumAdapters(0, &adapter);
	if (FAILED(result))
	{
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	displayModeList = new DXGI_MODE_DESC[numModes];
	if (!displayModeList)
	{
		return false;
	}

	// Now fill the display mode list structures.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if (FAILED(result))
	{
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	for (i = 0; i < numModes; i++)
	{
		if (displayModeList[i].Width == (unsigned int)screenWidth)
		{
			if (displayModeList[i].Height == (unsigned int)screenHeight)
			{
				numerator = displayModeList[i].RefreshRate.Numerator;
				denominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	// Get the adapter (video card) description.
	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	m_videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	error = wcstombs_s(&stringLength, m_videoCardDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		return false;
	}

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = nullptr;

	// Release the adapter output.
	adapterOutput->Release();
	adapterOutput = nullptr;

	// Release the adapter.
	adapter->Release();
	adapter = nullptr;

	// Release the factory.
	factory->Release();
	factory = nullptr;

	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = screenWidth;
	swapChainDesc.BufferDesc.Height = screenHeight;

	// Set regular 32-bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	if (m_vsyncEnabled)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	if (fullscreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Create the swap chain, Direct3D device, and Direct3D device context.
	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &m_swapChain, &m_device, NULL, &m_deviceContext);
	if (FAILED(result))
	{
		return false;
	}

	// Get the pointer to the back buffer.
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if (FAILED(result))
	{
		return false;
	}

	// Create the render target view with the back buffer pointer.
	result = m_device->CreateRenderTargetView(backBufferPtr, NULL, &m_renderTargetView);
	if (FAILED(result))
	{
		return false;
	}

	// Release pointer to the back buffer as we no longer need it.
	backBufferPtr->Release();
	backBufferPtr = nullptr;

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	// nullptr means we don't have a depth stencil buffer, which is the case for a 2D renderer
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

	// Setup the raster description which will determine how and what polygons will be drawn.
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	result = m_device->CreateRasterizerState(&rasterDesc, &m_rasterState);
	if (FAILED(result))
	{
		return false;
	}

	// Now set the rasterizer state.
	m_deviceContext->RSSetState(m_rasterState);

	D3D11_BLEND_DESC blendDesc = {};
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ID3D11BlendState* blendState = nullptr;
	result = m_device->CreateBlendState(&blendDesc, &blendState);
	if (FAILED(result))
		return false;

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	m_deviceContext->OMSetBlendState(blendState, blendFactor, 0xFFFFFFFF);
	blendState->Release();

	D3D11_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	result = m_device->CreateSamplerState(&samplerDesc, &m_samplerState);
	if (FAILED(result))
		return false;

	// Setup the viewport for rendering.
	m_viewport.Width = (float)screenWidth;
	m_viewport.Height = (float)screenHeight;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;

	// Create the viewport.
	m_deviceContext->RSSetViewports(1, &m_viewport);

	m_camera = Camera();

	// Create the projection matrix for 2D rendering.
	m_camera.projectionMatrix = XMMatrixOrthographicLH(m_viewport.Width, m_viewport.Height, screenNear, screenDepth);

	// Initialize the world matrix to the identity matrix.
	m_shaderManager = new ShaderManager();
	result = m_shaderManager->Initialize(m_device, hwnd);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the shader manager", L"Error", MB_OK);
		return false;
	}

	m_framebuffer = new Framebuffer();
	result = m_framebuffer->Initialize(m_device, screenWidth, screenHeight);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the framebuffer", L"Error", MB_OK);
		return false;
	}

	m_lightFramebuffer = new Framebuffer();
	result = m_lightFramebuffer->Initialize(m_device, screenWidth, screenHeight);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize light framebuffer", L"Error", MB_OK);
		return false;
	}

	m_finalFramebuffer = new Framebuffer();
	result = m_finalFramebuffer->Initialize(m_device, screenWidth, screenHeight);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize final framebuffer", L"Error", MB_OK);
		return false;
	}

	InitializeFullscreenQuad();


	m_spriteBatcher = new SpriteBatcher();
	result = m_spriteBatcher->Initialize(m_device, m_deviceContext);
	if (!result)
	{
		MessageBox(hwnd, L"Could not initialize the sprite batcher", L"Error", MB_OK);
		return false;
	}

	m_textureAtlas = new TextureAtlas();
	if (!m_textureAtlas->Initialize(m_device, m_deviceContext, ATLAS_MAX_SIZE, ATLAS_MAX_SIZE))
		return false;

	m_mvpCb.Init(m_device);
	
	BufferType::LightBufferType lightData = {};
	lightData.lights[0] = { XMFLOAT3(0.0f, 0.0f, -1.0f), 0.1f, XMFLOAT3(1.0f, 1.0f, 1.0f), 0.0f };
	lightData.lights[1] = { XMFLOAT3(100.0f, 0.0f, 0.0f), 20000.0f, XMFLOAT3(1.0f, 0.0f, 1.0f), 1.0f };
	/*lightData.lights[2] = { XMFLOAT3(-500.0f, 0.0f, 0.0f), 10000.0f, XMFLOAT3(1.0f, 1.0f, 1.0f), 1.0f };
	lightData.lights[3] = { XMFLOAT3(0.0f, 700.0f, 0.0f), 5000.0f, XMFLOAT3(0.0f, 1.0f, 1.0f), 1.0f };
	lightData.lights[4] = { XMFLOAT3(200.0f, 100.0f, 0.0f), 7000.0f, XMFLOAT3(1.0f, 0.5f, 0.0f), 1.0f };*/
	lightData.lightCount = 2;

	m_lightCb.Init(m_device);
	m_lightCb.Update(m_deviceContext, lightData);

	return true;
}

bool DirectX2D::InitializeFullscreenQuad()
{
	Vertex vertices[] =
	{
		{ XMFLOAT3(-1.0f,  1.0f, 0.5f), XMFLOAT2(0.0f, 0.0f), XMFLOAT4(1,1,1,1) },
		{ XMFLOAT3(1.0f,  1.0f, 0.5f), XMFLOAT2(1.0f, 0.0f), XMFLOAT4(1,1,1,1) },
		{ XMFLOAT3(1.0f, -1.0f, 0.5f), XMFLOAT2(1.0f, 1.0f), XMFLOAT4(1,1,1,1) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.5f), XMFLOAT2(0.0f, 1.0f), XMFLOAT4(1,1,1,1) },
	};

	unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.ByteWidth = sizeof(Vertex) * 4;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices;

	HRESULT result = m_device->CreateBuffer(&vbDesc, &vbData, &m_fullscreenQuadVB);
	if (FAILED(result)) return false;

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = sizeof(unsigned int) * 6;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	result = m_device->CreateBuffer(&ibDesc, &ibData, &m_fullscreenQuadIB);
	if (FAILED(result)) return false;

	return true;
}

void DirectX2D::Shutdown()
{
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	if (m_samplerState)
	{
		m_samplerState->Release();
		m_samplerState = nullptr;
	}

	if (m_textureAtlas)
	{
		m_textureAtlas->Shutdown();
		delete m_textureAtlas;
		m_textureAtlas = nullptr;
	}

	if (m_spriteBatcher)
	{
		m_spriteBatcher->Shutdown();
		delete m_spriteBatcher;
		m_spriteBatcher = nullptr;
	}

	if (m_fullscreenQuadVB) { m_fullscreenQuadVB->Release(); m_fullscreenQuadVB = nullptr; }
	if (m_fullscreenQuadIB) { m_fullscreenQuadIB->Release(); m_fullscreenQuadIB = nullptr; }

	if (m_finalFramebuffer)
	{
		m_finalFramebuffer->Shutdown();
		delete m_finalFramebuffer;
		m_finalFramebuffer = nullptr;
	}

	if (m_lightFramebuffer)
	{
		m_lightFramebuffer->Shutdown();
		delete m_lightFramebuffer;
		m_lightFramebuffer = nullptr;
	}

	if (m_framebuffer)
	{
		m_framebuffer->Shutdown();
		delete m_framebuffer;
		m_framebuffer = nullptr;
	}

	if (m_shaderManager)
	{
		m_shaderManager->Shutdown();
		delete m_shaderManager;
		m_shaderManager = nullptr;
	}

	if (m_rasterState)
	{
		m_rasterState->Release();
		m_rasterState = nullptr;
	}
	if (m_renderTargetView)
	{
		m_renderTargetView->Release();
		m_renderTargetView = nullptr;
	}
	if (m_deviceContext)
	{
		m_deviceContext->Release();
		m_deviceContext = nullptr;
	}
	if (m_device)
	{
		m_device->Release();
		m_device = nullptr;
	}
	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = nullptr;
	}
}

void DirectX2D::BeginScene(float red, float green, float blue, float alpha)
{
	float color[4];
	// Setup the color to clear the buffer to.
	color[0] = red;
	color[1] = green;
	color[2] = blue;
	color[3] = alpha;

	m_framebuffer->Bind(m_deviceContext);
	m_deviceContext->ClearRenderTargetView(m_framebuffer->GetRTV(), color);

	// Shader handles the rest

	m_shaderManager->GetActiveShader()->Bind(m_deviceContext);
	BufferType::MVPBufferType mvpData = { XMMatrixIdentity(), GetViewMatrix(), GetProjectionMatrix() };
	m_mvpCb.Update(m_deviceContext, mvpData.Transposed());
	m_mvpCb.BindVS(m_deviceContext, 0);

	ID3D11ShaderResourceView* srv = m_textureAtlas->GetSRV();
	m_deviceContext->PSSetShaderResources(0, 1, &srv);
	m_deviceContext->PSSetSamplers(0, 1, &m_samplerState);

	m_spriteBatcher->Begin();

}

void DirectX2D::EndScene()
{
	m_spriteBatcher->End();

	unsigned int stride = sizeof(Vertex);
	unsigned int offset = 0;

	// Render lights to light framebuffer

	m_shaderManager->GetShader(L"light")->Bind(m_deviceContext);

	float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_lightFramebuffer->Bind(m_deviceContext);
	m_deviceContext->ClearRenderTargetView(m_lightFramebuffer->GetRTV(), color);

	m_lightCb.BindPS(m_deviceContext, 1);

	BufferType::InvViewProjectionBufferType invVPData = { XMMatrixTranspose(XMMatrixInverse(nullptr, GetViewMatrix() * GetProjectionMatrix())), XMFLOAT2(m_viewport.Width, m_viewport.Height) };
	ConstantBuffer<BufferType::InvViewProjectionBufferType> invVPCB;																  
	invVPCB.Init(m_device);
	invVPCB.Update(m_deviceContext, invVPData);
	invVPCB.BindPS(m_deviceContext, 2);

	m_deviceContext->IASetVertexBuffers(0, 1, &m_fullscreenQuadVB, &stride, &offset);
	m_deviceContext->IASetIndexBuffer(m_fullscreenQuadIB, DXGI_FORMAT_R32_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_deviceContext->DrawIndexed(6, 0, 0);

	// Composite the light framebuffer with the main framebuffer

	m_finalFramebuffer->Bind(m_deviceContext);
	m_deviceContext->ClearRenderTargetView(m_finalFramebuffer->GetRTV(), color);

	m_shaderManager->GetShader(L"composite")->Bind(m_deviceContext);

	ID3D11ShaderResourceView* srvs[2] = { m_framebuffer->GetSRV(), m_lightFramebuffer->GetSRV() };
	m_deviceContext->PSSetShaderResources(0, 1, &srvs[0]);
	m_deviceContext->PSSetShaderResources(1, 1, &srvs[1]);
	m_deviceContext->PSSetSamplers(0, 1, &m_samplerState);

	m_deviceContext->IASetVertexBuffers(0, 1, &m_fullscreenQuadVB, &stride, &offset);
	m_deviceContext->IASetIndexBuffer(m_fullscreenQuadIB, DXGI_FORMAT_R32_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_deviceContext->DrawIndexed(6, 0, 0);

	// final pass: render framebuffer to back buffer with post-processing shader
	// Unbind the framebuffer so we can render to the back buffer
	SetBackBufferRenderTarget();

	m_shaderManager->GetPostProcessShader()->Bind(m_deviceContext);

	ID3D11ShaderResourceView* srv = m_finalFramebuffer->GetSRV();
	m_deviceContext->PSSetShaderResources(0, 1, &srv);
	m_deviceContext->PSSetSamplers(0, 1, &m_samplerState);

	// Draw fullscreen quad
	m_deviceContext->IASetVertexBuffers(0, 1, &m_fullscreenQuadVB, &stride, &offset);
	m_deviceContext->IASetIndexBuffer(m_fullscreenQuadIB, DXGI_FORMAT_R32_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_deviceContext->DrawIndexed(6, 0, 0);

	// Present the back buffer to the screen since rendering is complete.
	if (m_vsyncEnabled)
	{
		// Lock to screen refresh rate.
		m_swapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible.
		m_swapChain->Present(0, 0);
	}
	return;
}

void DirectX2D::DrawSprite(const Sprite& sprite) { m_spriteBatcher->DrawSprite(sprite); }

int DirectX2D::AddTexture(const wchar_t* filename)
{
	return m_textureAtlas->AddTexture(filename);
}

ID3D11ShaderResourceView* DirectX2D::LoadTexture(const wchar_t* filename)
{
	ScratchImage image;
	HRESULT result = LoadFromWICFile(filename, WIC_FLAGS_NONE, nullptr, image);
	if (FAILED(result))
		return nullptr;

	ID3D11ShaderResourceView* srv = nullptr;
	result = CreateShaderResourceView(m_device, image.GetImages(), image.GetImageCount(), image.GetMetadata(), &srv);
	if (FAILED(result))
		return nullptr;

	return srv;
}

bool DirectX2D::BuildAtlas()
{
	return m_textureAtlas->Build();
}

ID3D11Device* DirectX2D::GetDevice()
{
	return m_device;
}

ID3D11DeviceContext* DirectX2D::GetDeviceContext()
{
	return m_deviceContext;
}

XMMATRIX DirectX2D::GetProjectionMatrix()
{
	return m_camera.projectionMatrix;
}

XMMATRIX DirectX2D::GetWorldMatrix()
{
	// returns the identity matrix for 2D rendering
	return XMMatrixIdentity();
}

XMMATRIX DirectX2D::GetViewMatrix()
{
	return XMMatrixInverse(nullptr, m_camera.transform.GetWorld());
}

void DirectX2D::GetVideoCardInfo(char* cardName, int& memory)
{
	strcpy_s(cardName, 128, m_videoCardDescription);
	memory = m_videoCardMemory;
	return;
}

void DirectX2D::SetBackBufferRenderTarget()
{
	// Bind the render target view and depth stencil buffer to the output render pipeline.
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, nullptr);

	return;
}

void DirectX2D::ResetViewport()
{
	// Set the viewport.
	m_deviceContext->RSSetViewports(1, &m_viewport);

	return;
}
