#pragma once

#include <directxmath.h>

using namespace DirectX;

namespace clvr
{
	struct Sprite
	{
		XMFLOAT2 position;
		XMFLOAT2 size;
		XMFLOAT4 color;
		XMFLOAT4 uvRect; // x, y = top-left in UV space; z, w = width, height in UV space
		XMFLOAT2 pivot;
		float rotation;
		unsigned int layer;
		int textureID = -1;
	};

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT2 uv;
		XMFLOAT4 color;
	};
}