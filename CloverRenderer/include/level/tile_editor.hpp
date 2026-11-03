#pragma once

#include "core/editor_window.hpp"
#include "core/ecs.hpp"
#include "level/tile.hpp"

#include <memory>

namespace clvr
{
	class TilesEditor : public System, public EditorWindow
	{
	public:
		TilesEditor();
		~TilesEditor();

		void Update(float dt) override;
		void Render() override;
		void Inspect(float dt) override;

		RunMode GetRunMode() const { return RunMode::Editing; }

		void OnPlayStart();
		void OnPlayStop();

		json Save();
		void Load(const json&);

		int priority = 24;
		std::string title = "Tiles Editor";

	protected:
		void DrawContents();
		void OnOpen() {}        
		void OnClose() {}

	private:
		void DrawTileGrid();
		void DrawTileProperties();
		bool SpritePicker(TileDef& def);
		void SelectTile(TileID id);   // asks first if the draft has unsaved edits
		void LoadDraft(TileID id);
		void AddTile();
		void RemoveSelectedTile();
		bool SaveSelectedTile();
		void StatusText();

		TileID m_selectedId = 0;      // 0 = nothing selected
		TileDef m_draft;              // working copy of the selected tile
		bool m_draftDirty = false;

		TileID m_pendingSelectId = 0; // selection waiting on the discard prompt
		TileID m_scrollToId = 0;      // grid scrolls here once (set after Add)
		bool m_discardPrompt = false;
		bool m_confirmRemove = false;

		std::string m_status;

		std::shared_ptr<TileDatabase> m_tileDatabase = nullptr;
	};
}