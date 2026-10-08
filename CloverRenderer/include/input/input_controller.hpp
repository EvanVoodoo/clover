#pragma once
#include <vector>
#include <memory>
#include "input_command.hpp"
#include "core/serialization.hpp"

namespace clvr
{
	using CommandList = std::vector<std::unique_ptr<InputCommand>>;

	enum class ControllerType { Player, AI };
	NLOHMANN_JSON_SERIALIZE_ENUM(ControllerType, { {ControllerType::Player, "player"}, {ControllerType::AI, "ai"} })

	struct Controlled 
	{ 
		ControllerType type = ControllerType::Player; 
		void Inspect() {} 
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Controlled, type)

	class InputController
	{
	public:
		virtual ~InputController() = default;
		virtual void Produce(entt::registry& reg, Entity entity, CommandList& out) {}
	};

	class PlayerInputController : public InputController
	{
	public:
		void Produce(entt::registry&, entt::entity, CommandList& out) override;
	};
}

SAVEABLE_COMPONENT(clvr::Controlled, "controlled")