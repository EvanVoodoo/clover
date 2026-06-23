#pragma once

#include <directxmath.h>

using namespace DirectX;

namespace clvr
{
	struct Sprite
	{
		XMFLOAT2 position;
		XMFLOAT2 pivot;
		float rotation;
		XMFLOAT2 size;
		XMFLOAT4 color;
		unsigned int layer;
		int textureID = -1;
	};

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};
}