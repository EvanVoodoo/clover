#pragma once

#include "ecs.hpp"
#include <nlohmann/json.hpp>
#include "editor_window.hpp"
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

namespace clvr {

	class TranslateGizmo;
	class SelectionGizmo;

	using ChildMap = std::unordered_map<entt::entity, std::vector<entt::entity>>;

	class SceneManager : public System, public EditorWindow {
	public:
		SceneManager();
		~SceneManager() = default;

		void Update(float);
		void Render();
		void Draw();
		void Inspect(float);

		void Play();
		void Stop();

		void DeleteSelectedEntity();
		void UpdateSelectedEntity(entt::entity entity);

		Entity GetSelectedEntity() const { return m_selectedEntity; }
		Entity GetScrollEntity() const { return m_scrollEntity; }

		void SetScrollEntity(Entity entity) { m_scrollEntity = entity; }

		json SaveScene();
		void LoadScene(const json& scene);
		bool SaveSceneToFile(const std::filesystem::path& path);
		bool LoadSceneFromFile(const std::filesystem::path& path);

	protected:
		void DrawContents() override;
		void OnOpen() override;
		void OnClose() override;
		void DrawEntityNode(entt::entity entity, const ChildMap& children, bool& deleteRequested);

	private:
		Entity m_selectedEntity = entt::null;
		Entity m_scrollEntity = entt::null;
		std::unique_ptr<TranslateGizmo> m_translateGizmo = nullptr;
		std::unique_ptr<SelectionGizmo> m_selectionGizmo = nullptr;

		json m_snapshot;
		std::filesystem::path m_loadedScenePath;
	};
}