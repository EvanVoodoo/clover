#include "gravity_sensor.hpp"
#include "physics/physics_components.hpp"
#include <imgui.h>

void GravitySensorCommand::OnBegin(entt::registry& reg, const SensorContext& context)
{
	if (reg.valid(context.visitor)) {
		auto rb = reg.try_get<RigidBody>(context.visitor);
		if (rb)
		{
			originalGravityScales[context.visitor] = rb->gravityScale; // Store the original gravity scale
			rb->gravityScale = modifiedGravityScale;
		}
	}
}

void GravitySensorCommand::OnStay(entt::registry& reg, const SensorContext& context)
{
	return; // No action needed while staying in the sensor area
}

void GravitySensorCommand::OnEnd(entt::registry& reg, const SensorContext& context)
{
	if(reg.valid(context.visitor)) {
		auto rb = reg.try_get<RigidBody>(context.visitor);
		if (rb)
		{
			rb->gravityScale = originalGravityScales[context.visitor];
			originalGravityScales.erase(context.visitor); // Remove the stored original gravity scale
		}
	}
}

void GravitySensorCommand::Inspect()
{
	ImGui::DragFloat("Modified Gravity Scale", &modifiedGravityScale, 0.1f, -10.0f, 10.0f);
}