#pragma once
#include "serialization.hpp"
#include <DirectXMath.h>

namespace clvr
{
	struct PlayerComponent {
		float speed = 100.0f;
		float sprintScale = 2.0f;
		float crouchScale = 0.5f;
		void Inspect();
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PlayerComponent, speed, sprintScale, crouchScale)

	struct CameraComponent {
		bool followTarget = false;
		float followSpeed = 5.0f;
		DirectX::XMFLOAT2 targetPosition = { 0.0f, 0.0f };
		Entity followEntity = entt::null;
		void Inspect();
	};
}

SAVEABLE_COMPONENT(clvr::PlayerComponent, "Player Component")
REGISTER_COMPONENT(clvr::CameraComponent, "Camera Component")