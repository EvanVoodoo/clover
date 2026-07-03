#pragma once

#include <d3d11.h>
#include <directxmath.h>
#include <core/transform.hpp>

using namespace DirectX;

namespace clvr
{
	struct Sprite
	{
		XMFLOAT2 position;
		XMFLOAT2 size;
		XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		XMFLOAT4 uvRect; // x, y = top-left in UV space; z, w = width, height in UV space
		XMFLOAT2 pivot;
		float rotation;
		unsigned int layer = 0;
		bool isOccluder = true; // if true, this sprite will be used for occlusion rendering
	};

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT2 uv;
		XMFLOAT4 color;
	};

	struct Light
	{
		XMFLOAT3 direction;
		float intensity;
		XMFLOAT3 color;
		float type; // 0 = directional, 1 = point, 2 = spotlight
	};

	namespace BufferType
	{
		struct MVPBufferType
		{
			XMMATRIX world;
			XMMATRIX view;
			XMMATRIX projection;

			MVPBufferType Transposed() { return { XMMatrixTranspose(world), XMMatrixTranspose(view), XMMatrixTranspose(projection) }; }
		};
		struct LightBufferType
		{
			Light lights[16]; // MAX_LIGHTS = 16
			int lightCount;
			XMFLOAT3 _pad;
		};
		struct InvViewProjectionBufferType
		{
			XMMATRIX invViewProjection;
			XMFLOAT2 screenSize;
			XMFLOAT2 _pad;
		};
		static_assert(sizeof(MVPBufferType) % 16 == 0, "MatrixBufferType must be 16-byte aligned");
		static_assert(sizeof(LightBufferType) % 16 == 0, "LightBufferType must be 16-byte aligned");
		static_assert(sizeof(InvViewProjectionBufferType) % 16 == 0, "InvViewProjectionBufferType must be 16-byte aligned");
	}

	struct Camera
	{
		Transform transform;
		XMMATRIX projectionMatrix;
		float speed = 500.0f; // units per second

		XMMATRIX GetProjectionMatrix()
		{
			return projectionMatrix;
		}

		XMMATRIX GetWorldMatrix()
		{
			// returns the identity matrix for 2D rendering
			return XMMatrixIdentity();
		}

		XMMATRIX GetViewMatrix()
		{
			return XMMatrixInverse(nullptr, transform.GetWorld());
		}

		BufferType::MVPBufferType GetMVPBufferData()
		{
			return { GetWorldMatrix(), GetViewMatrix(), GetProjectionMatrix() };
		}
	};

	template<typename T>
	class ConstantBuffer
	{
	public:
		~ConstantBuffer()
		{
			if (m_buffer)
			{
				m_buffer->Release();
				m_buffer = nullptr;
			}
		}
		bool Init(ID3D11Device* device)
		{
			static_assert(sizeof(T) % 16 == 0, "cbuffer must be 16-byte aligned");
			D3D11_BUFFER_DESC d = {};
			d.Usage = D3D11_USAGE_DYNAMIC;
			d.ByteWidth = sizeof(T);
			d.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			return SUCCEEDED(device->CreateBuffer(&d, nullptr, &m_buffer));
		}

		void Update(ID3D11DeviceContext* ctx, const T& data)
		{
			D3D11_MAPPED_SUBRESOURCE m = {};
			ctx->Map(m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &m);
			memcpy(m.pData, &data, sizeof(T));
			ctx->Unmap(m_buffer, 0);
		}

		void BindVS(ID3D11DeviceContext* ctx, UINT slot) { ctx->VSSetConstantBuffers(slot, 1, &m_buffer); }
		void BindPS(ID3D11DeviceContext* ctx, UINT slot) { ctx->PSSetConstantBuffers(slot, 1, &m_buffer); }

	private:
		ID3D11Buffer* m_buffer = nullptr;
	};
}