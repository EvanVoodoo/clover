#include "level/door_sensor_command.hpp"
#include <imgui.h>
#include <core/components.hpp>
#include <core/engine.hpp>
#include <level/dungeon.hpp>
#include <level/common.hpp>

namespace clvr {
	void DoorSensorCommand::OnBegin(entt::registry& reg, const SensorContext& context)
	{
		if (reg.valid(context.visitor)) {
			auto player = reg.try_get<PlayerComponent>(context.visitor);
			if (player) {
				auto door = reg.try_get<DoorComponent>(context.sensor);
				auto ecs = Engine.GetECS();
				DungeonRuntime& dR = ecs->GetSystem<DungeonRuntime>();
				dR.MoveToRoom(dR.GetCurrentRoom(), door->type, context.visitor) ;
			}
		}
	}
	void DoorSensorCommand::OnStay(entt::registry& reg, const SensorContext& context)
	{
		// No action needed while staying in the sensor area
	}
	void DoorSensorCommand::OnEnd(entt::registry& reg, const SensorContext& context)
	{
		// No action needed while leaving the sensor area
	}
	void DoorSensorCommand::Inspect()
	{
	}
}
