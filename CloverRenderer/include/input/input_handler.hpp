#pragma once

#include "input.hpp"
#include "core/ecs.hpp"

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
	};
}