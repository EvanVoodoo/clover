#pragma once

#include <directxmath.h>
#include <string>
#include "core/serialization.hpp"
#include <vector>
#include <entt/entity/entity.hpp>

using namespace DirectX;

namespace clvr
{
	struct Transform
	{
		std::string name = "";
		XMFLOAT2 position;
		XMFLOAT2 scale;
		float rotation;
		entt::entity parent = entt::null;

		Transform() : position(0.0f, 0.0f), scale(1.0f, 1.0f), rotation(0.0f) {}
		XMMATRIX GetLocal() const {
			XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, 0.0f);
			XMMATRIX rotationMatrix = XMMatrixRotationZ(rotation);
			XMMATRIX scaleMatrix = XMMatrixScaling(scale.x, scale.y, 1.0f);
			XMMATRIX worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
			return worldMatrix;
		}
		XMMATRIX GetWorld() const;
		XMMATRIX GetWorld(const entt::registry& registry) const;

		XMFLOAT2 GetWorldPosition() const {
			XMMATRIX worldMatrix = GetWorld();
			XMFLOAT3 worldPos;
			XMStoreFloat3(&worldPos, worldMatrix.r[3]);
			return XMFLOAT2(worldPos.x, worldPos.y);
		}
		XMFLOAT2 GetWorldScale() const {
			XMMATRIX worldMatrix = GetWorld();
			XMFLOAT3 worldScale;
			XMStoreFloat3(&worldScale, worldMatrix.r[0]);
			return XMFLOAT2(worldScale.x, worldScale.y);
		}
		float GetWorldRotation() const {
			XMMATRIX worldMatrix = GetWorld();
			XMFLOAT4 worldQuat;
			XMStoreFloat4(&worldQuat, XMQuaternionRotationMatrix(worldMatrix));
			return 2.0f * std::atan2(worldQuat.z, worldQuat.w); // quaternion about Z -> angle
		}

		XMFLOAT2 GetLocalPosition() const { return position; }
		XMFLOAT2 GetLocalScale() const { return scale; }
		float GetLocalRotation() const { return rotation; }

		void Inspect();
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Transform, name, position, scale, rotation)

	bool IsAncestorOrSelf(entt::entity ancestor, entt::entity node);
	bool SetParent(entt::entity child, entt::entity newParent, bool keepWorld = true);
	std::vector<entt::entity> GetChildren(entt::entity parent);
	void DestroyHierarchy(entt::entity root);
}

SAVEABLE_COMPONENT_ORDERED(clvr::Transform, "Transform", 0)