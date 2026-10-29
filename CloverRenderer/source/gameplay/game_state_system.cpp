#include "gameplay/game_state_system.hpp"
#include <core/engine.hpp>
#include <core/components.hpp>
#include <gameplay/health.hpp>

namespace clvr {
	GameStateSystem::GameStateSystem() {
		title = "Game State System";
		priority = 100;
	}

	void GameStateSystem::Update(float dt)
	{
		Engine.GetECS()->GetDispatcher().update();
	}

	void GameStateSystem::Render()
	{
	}

	void GameStateSystem::Inspect(float dt)
	{
	}

	void GameStateSystem::OnPlayStart()
	{
		Engine.GetECS()->Listen<EntityDied, &GameStateSystem::OnEntityDied>(*this);
	}

	void GameStateSystem::OnPlayStop()
	{
		Engine.GetECS()->StopListening<GameStateSystem>(*this);
	}

	json GameStateSystem::Save()
	{
		return json();
	}

	void GameStateSystem::Load(const json&)
	{
	}

	void GameStateSystem::OnEntityDied(const EntityDied& event)
	{
		auto ecs = Engine.GetECS();
		auto& reg = ecs->GetRegistry();

		if (!reg.valid(event.entity)) {
			return;
		}

		if (reg.all_of<HealthComponent>(event.entity)) {
			auto& health = reg.get<HealthComponent>(event.entity);
			if (health.alive) {
				health.alive = false;
			}

			if (reg.all_of<PlayerComponent>(event.entity)) {
				ecs->DeleteEntity(event.entity);
			}
		}
	}
}
