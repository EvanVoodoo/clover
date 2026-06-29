#pragma once

#include <directxmath.h>

using namespace DirectX;

namespace clvr
{
	struct Transform
	{
		XMFLOAT2 position;
		XMFLOAT2 scale;
		float rotation;
		Transform() : position(000.0f, 0.0f), scale(1.0f, 1.0f), rotation(0.0f) {}
		XMMATRIX GetWorld() {
			XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, 0.0f);
			XMMATRIX rotationMatrix = XMMatrixRotationZ(rotation);
			XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, 1.0f);
			XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
			return worldMatrix;
		}
	};
}