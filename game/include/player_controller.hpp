#pragma once

#include "input/input_controller.hpp"
#include <input/input_command.hpp>

using namespace clvr;

struct PhysicsMoveCommand : InputCommand
{
	float dirX, dirY;                          // unit vector
	MoveMode mode;
	PhysicsMoveCommand(float x, float y, MoveMode m) : dirX(x), dirY(y), mode(m) {}
	void Execute(CommandContext& ctx, entt::entity actor) override;
};

class PhysicsPlayerController : public InputController
{
public:
	void Produce(entt::registry& reg, entt::entity entity, CommandList& out) override;
};