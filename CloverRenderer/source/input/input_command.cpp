#include "input/input_command.hpp"
#include "core/transform.hpp"
#include <core/components.hpp>

using namespace clvr;

void MoveCommand::Execute(CommandContext& ctx, entt::entity actor)
{
    auto* tf = ctx.reg.try_get<Transform>(actor);
    auto* player = ctx.reg.try_get<PlayerComponent>(actor);   // speed for now
    if (!tf || !player) return;                               // actor lost a component: do nothing
    tf->position.x += dirX * player->speed * ctx.dt;
    tf->position.y += dirY * player->speed * ctx.dt;
}
