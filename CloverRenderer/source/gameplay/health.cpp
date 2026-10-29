#include "gameplay/health.hpp"
#include <imgui.h>

namespace clvr {
	struct EntityDied {
		Entity entity;
	};

	void HealthComponent::Inspect()
	{
		if (ImGui::TreeNode("Health Component")) {
			ImGui::DragFloat("Current Health", &currentHealth, 1.0f, 0, maxHealth);
			ImGui::DragFloat("Max Health", &maxHealth, 1.0f, 1, 10000);
			ImGui::Checkbox("Alive", &alive);
			ImGui::Checkbox("Invincible", &invincible);
			ImGui::TreePop();
		}
	}
}