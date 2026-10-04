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

		void DeleteSelectedEntity();
		void UpdateSelectedEntity(entt::entity entity);

		Entity GetSelectedEntity() const { return m_selectedEntity; }
		Entity GetScrollEntity() const { return m_scrollEntity; }

		void SetScrollEntity(Entity entity) { m_scrollEntity = entity; }

	private:
		Entity m_selectedEntity = entt::null;
		Entity m_scrollEntity = entt::null;
		std::unique_ptr<TranslateGizmo> m_translateGizmo = nullptr;
	};
}