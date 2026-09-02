#pragma once

#include "ecs.hpp"

namespace clvr {
	class SceneManager : public System {
	public:
		SceneManager() = default;
		~SceneManager() = default;

		void Update(float);
		void Render();
		void Inspect(float);
	};
}