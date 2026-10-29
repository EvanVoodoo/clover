#pragma once

#include "core/serialization.hpp"
#include "core/ecs.hpp"
#include "health.hpp"

namespace clvr {
	enum class DamageType {
		Physical,
		True,
	};

	struct DamageEvent {
		Entity target;
		Entity source;
		float amount;
		DamageType type;
	};

	class DamageSystem : public System {
	public:
		// System overrides
		DamageSystem();
		~DamageSystem();
		void Update(float);
		void Render();
		void Inspect(float);

		RunMode GetRunMode() const { return RunMode::Playing; }

		void OnPlayStart();
		void OnPlayStop();

		json Save() { return json(); }
		void Load(const json&) {}

		// Damage system methods
		void QueueDamage(Entity target, Entity source, float amount, DamageType type);
		void QueueDeath(Entity target);
		void ClearDamageQueue();
		void ClearDeathQueue();
		void ProcessDamageQueue();
		void ProcessDeathQueue();

		void ApplyDamage(HealthComponent& target, float amount, DamageType type);
		void HealEntity(HealthComponent& target, float amount);
		void KillEntity(HealthComponent& target, Entity targetEntity);
	private:
		std::unordered_map<Entity, std::vector<DamageEvent>> m_damageQueue;
		std::vector<Entity> m_deathQueue;
	};
}