#pragma once

#include "input/input_controller.hpp"
#include <input/input_command.hpp>

using namespace clvr;

struct PhysicsMoveCommand : InputCommand
{
	float dirX = 0, dirY = 0;                          // unit vector
	bool jump = false;
	MoveMode mode;
	PhysicsMoveCommand(float x, float y, bool j, MoveMode m) : dirX(x), dirY(y), jump(j), mode(m) {}
	void Execute(CommandContext& ctx, entt::entity actor) override;
};

class PhysicsPlayerController : public InputController
{
public:
	void Produce(entt::registry& reg, entt::entity entity, CommandList& out) override;
};