#pragma once

#include "core/serialization.hpp"

namespace clvr {
	struct HealthComponent {
		HealthComponent() = default;
		HealthComponent(float current, float max) : currentHealth(current), maxHealth(max) {}
		float currentHealth = 100;
		float maxHealth = 100;
		bool alive = true;
		bool invincible = false;
		
		void Inspect();
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HealthComponent, currentHealth, maxHealth, alive, invincible)
}

SAVEABLE_COMPONENT_ORDERED(clvr::HealthComponent, "Health Component", 100)