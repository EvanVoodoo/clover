#pragma once

#include "ecs.hpp"
#include "translate_gizmo.hpp"

namespace clvr {
	class SceneManager : public System {
	public:
		SceneManager();
		~SceneManager() = default;

		void Update(float);
		void Render();
		void Draw();
		void Inspect(float);

		void UpdateSelectedEntity(entt::entity entity);

	private:
		Entity m_selectedEntity = entt::null;
		std::unique_ptr<TranslateGizmo> m_translateGizmo = nullptr;
	};
}