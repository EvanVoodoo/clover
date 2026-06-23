#include "rendering/sprite_batcher.hpp"

using namespace clvr;

SpriteBatcher::SpriteBatcher()
	: m_device(nullptr), m_deviceContext(nullptr),
	m_vertexBuffer(nullptr), m_indexBuffer(nullptr),
	m_vertexBufferBase(nullptr), m_vertexBufferPtr(nullptr),
	m_indexCount(0)
{
}

SpriteBatcher::SpriteBatcher(const SpriteBatcher& other)
{
	(void)other;
}

SpriteBatcher::~SpriteBatcher()
{
}

bool SpriteBatcher::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	m_device = device;
	m_deviceContext = deviceContext;

	m_vertexBufferBase = new Vertex[MAX_VERTICES];
	m_vertexBufferPtr = m_vertexBufferBase;

	if (!InitializeBuffers())
		return false;
	return true;
}

bool SpriteBatcher::InitializeBuffers()
{
	// Create vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc = {};
	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * MAX_VERTICES;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT result = m_device->CreateBuffer(&vertexBufferDesc, nullptr, &m_vertexBuffer);
	if (FAILED(result))
		return false;

	// Create index buffer
	D3D11_BUFFER_DESC indexBufferDesc = {};
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned int) * MAX_INDICES;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	unsigned int* indices = new unsigned int[MAX_INDICES];
	for (int i = 0; i < MAX_SPRITES; ++i)
	{
		indices[i * 6 + 0] = i * 4 + 0;
		indices[i * 6 + 1] = i * 4 + 1;
		indices[i * 6 + 2] = i * 4 + 2;
		indices[i * 6 + 3] = i * 4 + 0;
		indices[i * 6 + 4] = i * 4 + 2;
		indices[i * 6 + 5] = i * 4 + 3;
	}

	D3D11_SUBRESOURCE_DATA indexData = {};
	indexData.pSysMem = indices;

	result = m_device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	delete[] indices;
	if (FAILED(result))
		return false;
	return true;
}

void SpriteBatcher::Shutdown()
{
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = nullptr;
	}
	if (m_vertexBufferBase)
	{
		delete[] m_vertexBufferBase;
		m_vertexBufferBase = nullptr;
	}
}

void SpriteBatcher::Begin()
{
	m_vertexBufferPtr = m_vertexBufferBase;
	m_indexCount = 0;
}

void SpriteBatcher::DrawSprite(const Sprite& sprite)
{
	if (m_indexCount >= MAX_INDICES)
		return;

	float halfWidth = sprite.size.x * 0.5f;
	float halfHeight = sprite.size.y * 0.5f;

	float uvLeft = sprite.uvRect.x;
	float uvTop = sprite.uvRect.y;
	float uvRight = sprite.uvRect.x + sprite.uvRect.z;
	float uvBottom = sprite.uvRect.y + sprite.uvRect.w;

	XMFLOAT2 corners[4] = {
		{ -halfWidth, -halfHeight },
		{  halfWidth, -halfHeight },
		{  halfWidth,  halfHeight },
		{ -halfWidth,  halfHeight }
	};

	XMFLOAT2 uvs[4] = {
		{ uvLeft,  uvBottom },
		{ uvRight, uvBottom },
		{ uvRight, uvTop    },
		{ uvLeft,  uvTop    }
	};

	for (int i = 0; i < 4; ++i)
	{
		m_vertexBufferPtr->position = XMFLOAT3(sprite.position.x + corners[i].x, sprite.position.y + corners[i].y, 0.5f);
		m_vertexBufferPtr->uv = uvs[i];
		m_vertexBufferPtr->color = sprite.color;
		m_vertexBufferPtr++;
	}

	m_indexCount += 6;
}

void SpriteBatcher::End()
{
	if (m_indexCount == 0)
		return;

	// Map the vertex buffer and copy the vertices
	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	m_deviceContext->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, m_vertexBufferBase, sizeof(Vertex) * (m_indexCount / 6) * 4);

	m_deviceContext->Unmap(m_vertexBuffer, 0);

	// Set the vertex and index buffers
	unsigned int stride = sizeof(Vertex);
	unsigned int offset = 0;
	m_deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	m_deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_deviceContext->DrawIndexed(m_indexCount, 0, 0);
}