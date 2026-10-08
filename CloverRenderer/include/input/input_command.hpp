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

    enum class MoveMode { Walk, Sprint, Crouch };

    struct MoveCommand : InputCommand {
        float dirX, dirY;                          // unit vector
        MoveMode mode;
        MoveCommand(float x, float y, MoveMode m) : dirX(x), dirY(y), mode(m) {}

        void Execute(CommandContext& ctx, entt::entity actor) override;
    };
}