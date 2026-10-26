#pragma once

#include "physics/sensor_component.hpp"

using namespace clvr;

class GravitySensorCommand : public SensorCommand {
public:
	GravitySensorCommand() {}
	void OnBegin(entt::registry& reg, const SensorContext& context) override;
	void OnStay(entt::registry& reg, const SensorContext& context) override;
	void OnEnd(entt::registry& reg, const SensorContext& context) override;

	const char* GetTypeName() const override { return "GravitySensorCommand"; }

	void Inspect() override;

	void ToJson(nlohmann::json& j) const override {
		j["modifiedGravityScale"] = modifiedGravityScale;
	}
	void FromJson(const nlohmann::json& j) override {
		if (j.contains("modifiedGravityScale")) {
			modifiedGravityScale = j["modifiedGravityScale"].get<float>();
		}
	}

private:
	std::unordered_map<entt::entity, float> originalGravityScales; // Store original gravity scales for entities
	float modifiedGravityScale = 0.0f; // Gravity scale to set when in the sensor area
};