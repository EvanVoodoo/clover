#include "physics/collision_filters.hpp"
#include <imgui.h>

using namespace clvr;
using namespace clvr::CollisionFilters;

void clvr::CollisionFilters::InspectCollisionCategory(CollisionCategory& category) {
	if (ImGui::BeginCombo("Collision Category", ToString(category).data())) {
		for (const auto& rule : kCollisionTable) {
			const bool isSelected = (category == rule.category);
			if (ImGui::Selectable(ToString(rule.category).data(), isSelected)) {
				category = rule.category;
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
}