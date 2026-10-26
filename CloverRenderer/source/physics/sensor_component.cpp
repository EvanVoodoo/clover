#include "physics/sensor_component.hpp"
#include <rendering/render_components.hpp>
#include <algorithm>

namespace clvr
{
	void ColorSensorCommand::OnBegin(entt::registry& reg, const SensorContext& context)
	{
		if (!reg.valid(context.visitor))
			return;

		SpriteComponent* spriteComp = reg.try_get<SpriteComponent>(context.visitor);
		if (spriteComp)
		{
			auto& sprite = spriteComp->sprite;
			sprite.color = { modifiedColor[0], modifiedColor[1], modifiedColor[2], modifiedColor[3] }; // Change color to specified color on begin
		}
	}
	void ColorSensorCommand::OnStay(entt::registry& reg, const SensorContext& context)
	{
		return; // No action needed while staying in the sensor area
	}
	void ColorSensorCommand::OnEnd(entt::registry& reg, const SensorContext& context)
	{
		if (!reg.valid(context.visitor))
			return;

		SpriteComponent* spriteComp = reg.try_get<SpriteComponent>(context.visitor);
		if (spriteComp)
		{
			auto& sprite = spriteComp->sprite;
			sprite.color = { 1.0f, 1.0f, 1.0f, 1.0f }; // Change color back to white on end
		}
	}

	void ColorSensorCommand::Inspect() {
		ImGui::ColorEdit4("Modified Color", modifiedColor);
	}

	void SensorComponent::Inspect()
	{
		if (!ImGui::TreeNode("SensorComponent"))
			return;

		ImGui::Checkbox("Active", &active);

		// Existing commands
		int removeIndex = -1;
		for (size_t i = 0; i < commands.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			if (ImGui::TreeNode(commands[i]->GetTypeName()))
			{
				commands[i]->Inspect();
				if (ImGui::Button("Remove"))
					removeIndex = static_cast<int>(i);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		if (removeIndex >= 0) // erase after the loop, not during it
			commands.erase(commands.begin() + removeIndex);

		// Add a command
		static int selected = 0;
		const auto names = SensorCommandFactory::Get().GetNames();
		if (!names.empty())
		{
			selected = (std::min) (selected, static_cast<int>(names.size()) - 1);
			if (ImGui::BeginCombo("##newCommand", names[selected].c_str()))
			{
				for (int n = 0; n < static_cast<int>(names.size()); ++n)
					if (ImGui::Selectable(names[n].c_str(), n == selected))
						selected = n;
				ImGui::EndCombo();
			}
			ImGui::SameLine();
			if (ImGui::Button("Add"))
			{
				if (auto cmd = SensorCommandFactory::Get().Create(names[selected]))
					commands.push_back(std::move(cmd));
			}
		}

		ImGui::Text("Overlapping Entities:");
		for (Entity e : overlapping)
			ImGui::Text("Entity ID: %u", static_cast<unsigned>(entt::to_integral(e)));

		ImGui::TreePop();
	}

	void SensorComponent::OnBegin(entt::registry& reg, Entity visitor) {
		if (!active) return;

		overlapping.push_back(visitor);
		SensorContext context{ visitor };
		for (auto& command : commands) {
			command->OnBegin(reg, context);
		}
	}
	void SensorComponent::OnStay(entt::registry& reg, Entity visitor) {
		if (!active) return;

		for (Entity overlappingEntity : overlapping) {
			if (!reg.valid(overlappingEntity)) {
				// Remove invalid entity from overlapping list
				overlapping.erase(std::remove(overlapping.begin(), overlapping.end(), overlappingEntity), overlapping.end());
				continue;
			}
			SensorContext context{ overlappingEntity };
			for (auto& command : commands) {
				command->OnStay(reg, context);
			}
		}
	}
	void SensorComponent::OnEnd(entt::registry& reg, Entity visitor) {
		if (!active) return;

		overlapping.erase(std::remove(overlapping.begin(), overlapping.end(), visitor), overlapping.end());
		SensorContext context{ visitor };
		for (auto& command : commands) {
			command->OnEnd(reg, context);
		}
	}
}