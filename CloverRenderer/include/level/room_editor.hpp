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
	enum class LayerKind { Visual, Gameplay, Spawn };

	struct LayerSelection
	{
		LayerKind kind = LayerKind::Visual;
		int index = 0; // only meaningful for Visual
	};

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

		Grid<TileID>* ActiveTileGrid();

	private:
		void MenuBar();
		void DrawCanvas();
		void LayerPanel();

		// helpers
		void RefreshRoomList();
		void SetNameBuffer(const std::string& s);
		void RunUnlessDirty(std::function<void()> action);
		bool RoomExists(const std::string& name) const;
		void StatusText();

		std::shared_ptr<TileDatabase> m_tileDatabase = nullptr;

		RoomData m_currentRoom;
		LayerSelection m_activeLayer;
		std::vector<bool> m_visualLayerVisible; // one per visual layer
		bool m_showGameplay = true;
		bool m_showSpawns = true;
		int m_hoverX = -1;
		int m_hoverY = -1;

		int m_zoom = 1; // 1-6, where 1 is 1:1 and 6 is 6:1

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