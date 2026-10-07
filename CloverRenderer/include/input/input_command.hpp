#pragma once

#include "core/ecs.hpp"

namespace clvr
{
	struct CommandContext { entt::registry& reg; float dt; };

	class InputCommand
	{
	public:
		virtual ~InputCommand() = default;
		virtual void Execute(CommandContext& ctx, entt::entity entity) = 0;
	};

    struct MoveCommand : public InputCommand {
        float dirX, dirY;                                     // already normalized by the controller
        MoveCommand(float x, float y) : dirX(x), dirY(y) {}

        void Execute(CommandContext& ctx, entt::entity actor) override;
    };
}