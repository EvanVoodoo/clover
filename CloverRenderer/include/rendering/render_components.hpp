#pragma once

#include <d3d11.h>
#include <directxmath.h>
#include <core/transform.hpp>
#include <string>
#include <wrl/client.h>
#include "texture.hpp"
#include <memory>
#include <core/ecs.hpp>

#define MAX_LIGHTS 16

using namespace DirectX;

namespace clvr
{
	struct SpriteLayer
	{
		unsigned int id = 0;
		std::string layerName = "Sprite Layer";
		float parallaxFactor = 1.0f; // 1.0 = normal speed, <1.0 = slower, >1.0 = faster
	};

	struct Sprite
	{
		Sprite(const std::string& filename = "", bool loadTexture = false) {
			if (loadTexture) {
				LoadSpriteTexture(filename);
				textureName = texture->GetFilename();
			}
			else textureName = filename;

			color = { 1.0f, 1.0f, 1.0f, 1.0f };
			uvRect = { 0.0f, 0.0f, 1.0f, 1.0f };
		}
		bool LoadSpriteTexture(const std::string& filename);
		bool LoadSpriteTexture(const std::shared_ptr<Texture> texture);

		XMFLOAT2 position;
		XMFLOAT2 size;
		XMFLOAT4 color;
		XMFLOAT4 uvRect; // x, y = top-left in UV space; z, w = width, height in UV space
		XMFLOAT2 pivot;
		SpriteLayer* layer = nullptr;
		float rotation;
		bool isOccluder = true; // if true, this sprite will be used for occlusion rendering

		bool useLinkedTexture = false;
		std::shared_ptr<Texture> texture = nullptr; // set once, e.g. via LoadTexture function
		std::string textureName = "";
	};

	struct SpriteComponent
	{
		SpriteComponent();
		SpriteComponent(Sprite s);
		Sprite sprite;   // reuse your existing Sprite struct as the payload
		void Inspect();
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

		void Inspect();
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
			Light lights[MAX_LIGHTS];
			int lightCount;
			XMFLOAT3 _pad;
		};
		struct IDBufferType
		{
			XMMATRIX mvp;
			UINT entityID;
			XMFLOAT3 _pad; // pad to 16-byte boundary — cbuffers require 16-byte alignment
		};
		static_assert(sizeof(MVPBufferType) % 16 == 0, "MatrixBufferType must be 16-byte aligned");
		static_assert(sizeof(LightBufferType) % 16 == 0, "LightBufferType must be 16-byte aligned");
		static_assert(sizeof(IDBufferType) % 16 == 0, "IDBufferType must be 16-byte aligned");
	}

	struct Camera
	{
		Transform transform;
		float speed = 500.0f; // units per second

		float viewportWidth = 1920.0f;
		float viewportHeight = 1080.0f;
		float nearZ = 0.0f;
		float farZ = 1.0f;

		float zoom = 1.0f; // >1 = zoomed in, <1 = zoomed out

		XMMATRIX GetProjectionMatrix(bool noZoom = false)
		{
			if (noZoom)
				return XMMatrixOrthographicLH(viewportWidth, viewportHeight, nearZ, farZ);
			// dividing by zoom shrinks the visible world area as zoom increases
			return XMMatrixOrthographicLH(viewportWidth / zoom, viewportHeight / zoom, nearZ, farZ);
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

REGISTER_COMPONENT(clvr::SpriteComponent, "Sprite")
REGISTER_COMPONENT(clvr::Light, "Light")