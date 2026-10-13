#include "player_controller.hpp"
#include <core/engine.hpp>
#include <physics/physics_components.hpp>
#include <core/components.hpp>
#include <core/transform.hpp>
#include <physics/world.hpp>

using namespace clvr;

void PhysicsPlayerController::Produce(entt::registry& reg, entt::entity entity, CommandList& out)
{
    if (!Engine.GameWindowFocused()) return;

    const float x = float(IsKeyDown('D')) - float(IsKeyDown('A'));
    const float y = float(IsKeyDown('W')) - float(IsKeyDown('S'));
    const float len = std::sqrt(x * x + y * y);

    if (len > 0.f) {                                    // not an early return: discrete actions below still need to run
        const bool sprint = IsKeyDown(VK_SHIFT);
        const bool crouch = IsKeyDown(VK_CONTROL);
        MoveMode mode = MoveMode::Walk;
        if (sprint != crouch) mode = sprint ? MoveMode::Sprint : MoveMode::Crouch;
        out.push_back(std::make_unique<PhysicsMoveCommand>(x / len, y / len, mode));
    }
}

void PhysicsMoveCommand::Execute(CommandContext& ctx, entt::entity actor)
{
    auto* tf = ctx.reg.try_get<Transform>(actor);
    auto* p = ctx.reg.try_get<PlayerComponent>(actor);
	auto* rb = ctx.reg.try_get<RigidBody>(actor);
    if (!tf || !p || !rb) return;
    float scale = mode == MoveMode::Sprint ? p->sprintScale
        : mode == MoveMode::Crouch ? p->crouchScale : 1.f;
    World& world = Engine.GetECS()->GetSystem<World>();
	// move the player by aplying a force to the rigid body
	world.ApplyForce(*rb, b2Vec2(dirX * p->speed * scale, dirY * p->speed * scale), b2Body_GetPosition(rb->bodyId), true);
}
