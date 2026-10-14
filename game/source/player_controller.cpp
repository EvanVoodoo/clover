#include "player_controller.hpp"
#include <core/engine.hpp>
#include <physics/physics_components.hpp>
#include <core/components.hpp>
#include <core/transform.hpp>
#include <physics/world.hpp>
#include <Box2D/math_functions.h>

using namespace clvr;

void PhysicsPlayerController::Produce(entt::registry& reg, entt::entity entity, CommandList& out)
{
    float x = 0.f, y = 0.f;
    bool jump = false;

    if (Engine.GameWindowFocused())     // unfocused = no input, but still send the stop
    {
        x = float(IsKeyDown('D')) - float(IsKeyDown('A'));
        y = float(IsKeyDown('W')) - float(IsKeyDown('S'));
        jump = WasKeyJustPressed(VK_SPACE);
    }

    const float len = std::sqrt(x * x + y * y);
    MoveMode mode = MoveMode::Walk;

    if (len > 0.f)
    {
        const bool sprint = IsKeyDown(VK_SHIFT);
        const bool crouch = IsKeyDown(VK_CONTROL);
        if (sprint != crouch) mode = sprint ? MoveMode::Sprint : MoveMode::Crouch;
        x /= len;
        y /= len;
    }

    out.push_back(std::make_unique<PhysicsMoveCommand>(x, y, jump, mode));
}

void PhysicsMoveCommand::Execute(CommandContext& ctx, entt::entity actor)
{
    auto* p = ctx.reg.try_get<PlayerComponent>(actor);
    if (!p) return;

    b2Vec2 dir = { dirX, dirY };
    if (b2LengthSquared(dir) > 0.0f)
        dir = b2Normalize(dir);               // diagonals aren't faster

    p->moveInput = dir;
    p->moveScale = mode == MoveMode::Sprint ? p->sprintScale
        : mode == MoveMode::Crouch ? p->crouchScale : 1.0f;
}
