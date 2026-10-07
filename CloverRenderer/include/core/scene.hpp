#pragma once

#include "ecs.hpp"
#include "translate_gizmo.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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

		json SaveScene();
		void LoadScene(const json& scene);
		bool SaveSceneToFile(const std::filesystem::path& path);
		bool LoadSceneFromFile(const std::filesystem::path& path);

	private:
		Entity m_selectedEntity = entt::null;
		Entity m_scrollEntity = entt::null;
		std::unique_ptr<TranslateGizmo> m_translateGizmo = nullptr;
	};
}