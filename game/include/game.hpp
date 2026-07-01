#pragma once

#include "core/ecs.hpp"

using namespace clvr;

class Game : public System {
public:
	Game();
	void SetupRenderer();
	~Game() = default;

	void Update(float);
	void Render();

	int priority = 0;
	std::string title = "GameSystem";
};