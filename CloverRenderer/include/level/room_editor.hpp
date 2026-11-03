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

		void RoomProperties();
		void BackdropPicker();
		void SetBackdrop(BackdropID id);

		std::shared_ptr<TileDatabase> m_tileDatabase = nullptr;
		std::shared_ptr<BackdropDatabase> m_backdropDatabase = nullptr;

		RoomData m_currentRoom;
		LayerSelection m_activeLayer;
		std::vector<bool> m_visualLayerVisible; // one per visual layer
		bool m_showGameplay = true;
		bool m_showSpawns = true;
		int m_hoverX = -1;
		int m_hoverY = -1;
		bool m_showBackdrop = true;
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

		enum class PaintTool { Draw, Erase, Fill, Rectangle };

		// methods
		void ToolBar();
		void TilePalette();
		void CanvasInput(const ImVec2& origin, float cell);
		const char* PaintBlockedReason() const;
		void CancelStroke();

		void SetCell(Grid<TileID>& g, int x, int y, TileID id);
		void PaintLine(Grid<TileID>& g, int x0, int y0, int x1, int y1, TileID id);
		void FloodFill(Grid<TileID>& g, int x, int y, TileID id);
		void FillRect(Grid<TileID>& g, int x0, int y0, int x1, int y1, TileID id);

		PaintTool m_tool = PaintTool::Draw;
		TileID m_brushTile = 1;          // 0 = eraser

		bool m_stroking = false;         // draw/erase drag in progress
		int  m_lastX = 0, m_lastY = 0;   // last cell painted during the drag

		bool m_rectDragging = false;     // rectangle drag in progress
		int  m_rectX0 = 0, m_rectY0 = 0; // rectangle start cell
	};
}