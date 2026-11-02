#pragma once

#include "core/ecs.hpp"
#include "level/room.hpp"
#include "core/editor_window.hpp"

namespace clvr
{
	class RoomEditor : public System, public EditorWindow
	{
	public:
		RoomEditor();
		~RoomEditor();
		void Update(float dt) override;
		void Render() override;
		void Inspect(float dt) override;

		RunMode GetRunMode() const { return RunMode::Editing; }

		void OnPlayStart();
		void OnPlayStop();

		json Save();
		void Load(const json&);

		int priority = 25;
		std::string title = "Room Editor";

	protected:
		void DrawContents() override;
		void OnOpen() override;
		void OnClose() override;
	};
}