#include "gameplay/damage.hpp"
#include <core/engine.hpp>
#include <gameplay/health.hpp>
#include <gameplay/common.hpp>

namespace clvr {

	DamageSystem::DamageSystem()
	{
		title = "Damage System";
		priority = 95;
	}

	DamageSystem::~DamageSystem()
	{}

	void DamageSystem::Update(float dt)
	{
		ProcessDamageQueue();
		ProcessDeathQueue();
		ClearDamageQueue();
		ClearDeathQueue();
	}

	void DamageSystem::Render()
	{}

	void DamageSystem::Inspect(float dt)
	{}

	void DamageSystem::OnPlayStart()
	{
		ClearDeathQueue();
		ClearDamageQueue();
	}

	void DamageSystem::OnPlayStop()
	{
		ClearDeathQueue();
		ClearDamageQueue();
	}

	void DamageSystem::QueueDamage(Entity target, Entity source, float amount, DamageType type)
	{
		DamageEvent event{ target, source, amount, type };
		m_damageQueue[target].push_back(event);
	}

	void DamageSystem::QueueDeath(Entity target)
	{
		m_deathQueue.push_back(target);
	}

	void DamageSystem::ClearDamageQueue()
	{
		m_damageQueue.clear();
	}

	void DamageSystem::ClearDeathQueue()
	{
		m_deathQueue.clear();
	}

	void DamageSystem::ProcessDamageQueue()
	{
		if (m_damageQueue.empty()) return;
		for (auto& [target, events] : m_damageQueue)
		{
			for (auto& event : events)
			{
				auto hComp = Engine.GetECS()->GetRegistry().try_get<HealthComponent>(event.target); // Ensure the target has a Health component
				if (!hComp) {
					continue; // Skip if the target doesn't have a Health component
				}
				if (hComp->invincible || !hComp->alive) {
					continue; // Skip if the target is invincible
				}
				ApplyDamage(*hComp, event.amount, event.type);
				if (hComp->currentHealth <= 0) {
					QueueDeath(event.target);
				}
			}
		}
	}

	void DamageSystem::ProcessDeathQueue()
	{
		if (m_deathQueue.empty()) return;

		for (auto& target : m_deathQueue)
		{
			auto hComp = Engine.GetECS()->GetRegistry().try_get<HealthComponent>(target);
			if (!hComp) {
				continue; // Skip if the target doesn't have a Health component
			}
			if (hComp->alive) {
				KillEntity(*hComp, target);
			}
		}
	}

	void DamageSystem::ApplyDamage(HealthComponent& target, float amount, DamageType type)
	{
		if (target.invincible || !target.alive || amount < 0) {
			return;
		}
		target.currentHealth -= amount;
		if (target.currentHealth < 0) {
			target.currentHealth = 0;
		}
	}

	void DamageSystem::HealEntity(HealthComponent& target, float amount)
	{
		if (!target.alive || amount < 0) {
			return;
		}
		target.currentHealth += amount;
		if (target.currentHealth > target.maxHealth) {
			target.currentHealth = target.maxHealth;
		}
	}

	void DamageSystem::KillEntity(HealthComponent& target, Entity targetEntity)
	{
		if (!target.alive) {
			return;
		}
		if (target.invincible) {
			return;
		}
		target.alive = false;
		Engine.GetECS()->GetDispatcher().enqueue(EntityDied{ targetEntity });
	}
}