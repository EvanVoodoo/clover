#pragma once
#include "serialization.hpp"
#include <DirectXMath.h>
#include "physics/units.hpp"

namespace clvr
{
	struct PlayerComponent {
		float maxSpeed = 400.0f;    // px/s
		float acceleration = 3000.0f;   // px/s^2, ~0.13 s to full speed
		float deceleration = 4000.0f;   // px/s^2, a bit higher so stopping feels snappy
		float sprintScale = 2.0f;
		float crouchScale = 0.5f;

		b2Vec2 moveInput = { 0, 0 };  // runtime only, don't serialize
		float moveScale = 1.0f;      // runtime only (sprint/crouch)
		void Inspect();
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PlayerComponent, maxSpeed, acceleration, deceleration, sprintScale, crouchScale)

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