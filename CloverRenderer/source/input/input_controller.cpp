#include "input/input_controller.hpp"
#include <core/engine.hpp>

using namespace clvr;

void PlayerInputController::Produce(entt::registry&, entt::entity, CommandList& out)
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
        out.push_back(std::make_unique<MoveCommand>(x / len, y / len, mode));
    }

    // discrete actions: if (WasKeyJustPressed(VK_SPACE)) out.push_back(std::make_unique<AttackCommand>());
}