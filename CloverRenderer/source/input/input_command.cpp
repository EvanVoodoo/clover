#include "input/input_command.hpp"
#include "core/transform.hpp"
#include <core/components.hpp>

using namespace clvr;

void MoveCommand::Execute(CommandContext& ctx, entt::entity actor) {
    auto* tf = ctx.reg.try_get<Transform>(actor);
    auto* p = ctx.reg.try_get<PlayerComponent>(actor);
    if (!tf || !p) return;
    float scale = mode == MoveMode::Sprint ? p->sprintScale
        : mode == MoveMode::Crouch ? p->crouchScale : 1.f;
    tf->position.x += dirX * p->speed * scale * ctx.dt;
    tf->position.y += dirY * p->speed * scale * ctx.dt;
}
