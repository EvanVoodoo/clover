#pragma once

#include "core/ecs.hpp"
#include "gameplay/common.hpp"

namespace clvr
{
	class GameStateSystem : public System
	{
	public:
		// System overrides
		GameStateSystem();
		void Update(float dt) override;
		void Render() override;
		void Inspect(float dt) override;
		RunMode GetRunMode() const { return RunMode::Always; }
		void OnPlayStart() override;
		void OnPlayStop() override;
		json Save() override;
		void Load(const json&) override;

		// Game state management

		void StartGame() { m_gameStarted = true; }
		void EndGame() { m_gameStarted = false; }

		void OnEntityDied(const EntityDied& event);
	private:
		bool m_gameStarted = false;
	};
}