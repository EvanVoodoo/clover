#pragma once

#include "core/ecs.hpp"

using namespace clvr;

struct MovingLight {
	MovingLight() = default;
};

struct MovingSprite {
	MovingSprite() = default;
};

class Game : public System {
public:
	Game();
	~Game() = default;

	void Update(float);
	void Render();
	void Inspect(float);
	
	void OnPlayStart() override;

private:
	void SetupScene();

	float m_time = 0.0f;
};