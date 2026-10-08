#include "input/input_handler.hpp"
#include <input/input_controller.hpp>
#include <core/engine.hpp>

using namespace clvr;

InputHandler::InputHandler()
{
	m_controllers[ControllerType::Player] = std::make_unique<PlayerInputController>();
	priority = 100;  // high priority to process input first
}

InputHandler::~InputHandler()
{

}

void InputHandler::Update(float dt) 
{
	auto& registry = Engine.GetECS()->GetRegistry();

	m_frame.clear();
	CommandList tmpCommands;
	for (auto [entity, controlled] : registry.view<Controlled>().each())
	{
		auto it = m_controllers.find(controlled.type);
		if (it != m_controllers.end())
		{
			tmpCommands.clear();
			it->second->Produce(registry, entity, tmpCommands);
			for (auto& cmd : tmpCommands)
				m_frame.emplace_back(entity, std::move(cmd));
		}
	}

	CommandContext ctx{ registry, dt };
	for (auto& [entity, cmd] : m_frame)
		if (registry.valid(entity))
			cmd->Execute(ctx, entity);
}

void InputHandler::Render() {}

void InputHandler::Inspect(float dt) {}

json InputHandler::Save() { return json(); }

void InputHandler::Load(const json&) {}

inline void InputHandler::AddController(ControllerType type, std::unique_ptr<InputController> controller) {
	m_controllers[type] = std::move(controller);
}

inline void InputHandler::RemoveController(ControllerType type) {
	m_controllers.erase(type);
}

inline std::unordered_map<ControllerType, std::unique_ptr<InputController>>& InputHandler::GetControllers() {
	return m_controllers;
}

inline std::vector<std::pair<entt::entity, std::unique_ptr<InputCommand>>>& InputHandler::GetFrameCommands() {
	return m_frame;
}
