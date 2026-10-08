#pragma once

#include "input.hpp"
#include "core/ecs.hpp"
#include "input_controller.hpp"

namespace clvr
{
	class InputHandler : public System
	{
	public:
		InputHandler();
		~InputHandler();
		void Update(float);
		void Render();
		void Inspect(float);
		RunMode GetRunMode() const { return RunMode::Playing; }
		json Save();
		void Load(const json&);
		std::string title = "Input Handler";

		void AddController(ControllerType type, std::unique_ptr<InputController> controller);
		void RemoveController(ControllerType type);
		std::unordered_map<ControllerType, std::unique_ptr<InputController>>& GetControllers();
		std::vector<std::pair<entt::entity, std::unique_ptr<InputCommand>>>& GetFrameCommands();
	private:
		std::unordered_map<ControllerType, std::unique_ptr<InputController>> m_controllers;
		std::vector<std::pair<entt::entity, std::unique_ptr<InputCommand>>> m_frame;
	};
}