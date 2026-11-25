#pragma once
#include "physics/physics_components.hpp"
#include "physics/sensor_component.hpp"
#include "level/common.hpp"

namespace clvr {

	class DoorSensorCommand : public SensorCommand {
	public:
		DoorSensorCommand() {}
		void OnBegin(entt::registry& reg, const SensorContext& context) override;
		void OnStay(entt::registry& reg, const SensorContext& context) override;
		void OnEnd(entt::registry& reg, const SensorContext& context) override;

		const char* GetTypeName() const override { return "DoorSensorCommand"; }

		void Inspect() override;

		void ToJson(nlohmann::json& j) const override {
		}
		void FromJson(const nlohmann::json& j) override {
		}

	private:
	};

}