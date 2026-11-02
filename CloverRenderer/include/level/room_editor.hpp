#pragma once

#include "core/ecs.hpp"
#include "level/room.hpp"
#include "core/editor_window.hpp"
#include <vector>
#include <resources/resource_manager.hpp>
#include <memory>
#include "tile.hpp"
#include <functional>

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

		// EditorWindow functions

		std::filesystem::path RoomPath(const std::string& name) const;
		bool CreateRoom(const std::string& name);
		bool SaveRoom(const std::string& name);
		bool LoadRoom(const std::string& name);

	protected:
		void DrawContents() override;
		void OnOpen() override;
		void OnClose() override;

	private:
		void MenuBar();

		// helpers
		void RefreshRoomList();
		void SetNameBuffer(const std::string& s);
		void RunUnlessDirty(std::function<void()> action);
		bool RoomExists(const std::string& name) const;
		void StatusText();

		std::shared_ptr<TileDatabase> m_tileDatabase = nullptr;

		RoomData m_currentRoom;

		std::vector<FileInfo> m_roomFiles = {};
		bool m_openRoomSaver = false;
		bool m_openRoomLoader = false;
		bool m_newRoomCreator = false;
		char m_roomNameBuffer[128] = {};
		std::string m_currentRoomName;  // empty = no room open
		bool m_dirty = false;
		std::string m_status;

		bool m_confirmOverwrite = false;
		bool m_discardPrompt = false;
		bool m_discardConfirmed = false;
		std::function<void()> m_pendingAction;
	};
}