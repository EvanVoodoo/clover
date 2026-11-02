#include "core/editor_window.hpp"
#include "core/engine.hpp"

namespace clvr {
	void EditorWindowManager::DrawMenu()
	{
		if (ImGui::BeginMenu("Tools"))
		{
			for (EditorWindow* system : Engine.GetECS()->GetSystems<EditorWindow>())
			{
				if (ImGui::MenuItem(system->Title().c_str(), nullptr, system->IsOpen()))
					system->Toggle();
			}
			ImGui::EndMenu();
		}
	}
}