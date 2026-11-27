#include "level/tile_editor.hpp"
#include <core/engine.hpp>
#include "core/imgui_helpers.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include "level/tile.hpp"
#include <rendering/renderer.hpp>

namespace clvr
{
	TilesEditor::TilesEditor() : EditorWindow("Tiles Editor", ImGuiWindowFlags_MenuBar)
	{
		title = Title();

		m_tileDatabase = Engine.GetResourceManager()->Load<TileDatabase>(
			Engine.GetResourceManager()->GetPath(ResourceManager::Directory::Assets, "tiles/tiles.json").c_str());

		if (!m_tileDatabase)
		{
			// create a new tile database if it doesn't exist
			TileDatabase newTileDatabase(Engine.GetResourceManager()->GetPath(ResourceManager::Directory::Assets, "tiles/tiles.json").c_str());
			newTileDatabase.SaveToFile();
			m_tileDatabase = Engine.GetResourceManager()->Load<TileDatabase>(
				Engine.GetResourceManager()->GetPath(ResourceManager::Directory::Assets, "tiles/tiles.json").c_str());
		}
	}
	TilesEditor::~TilesEditor()
	{
	}
	void TilesEditor::Update(float dt)
	{
		m_tileDatabase->RefreshSprites();
	}
	void TilesEditor::Render()
	{
	}
	void TilesEditor::Inspect(float dt)
	{
		DrawWindow();
	}
	void TilesEditor::OnPlayStart()
	{}
	void TilesEditor::OnPlayStop()
	{}
	json TilesEditor::Save()
	{
		return json();
	}
	void TilesEditor::Load(const json&)
	{}

	namespace
	{
		bool InputString(const char* label, std::string& value)
		{
			char buf[128];
			std::snprintf(buf, sizeof(buf), "%s", value.c_str());
			if (ImGui::InputText(label, buf, sizeof(buf)))
			{
				value = buf;
				return true;
			}
			return false;
		}
	}

	void TilesEditor::DrawContents()
	{
		if (!m_tileDatabase)
		{
			ImGui::TextUnformatted("Tile database failed to load.");
			return;
		}

		// Left: every tile in the database. Drag the right edge to resize.
		ImGui::BeginChild("TileGrid", ImVec2(560.0f, 0),
						  ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
		DrawTileGrid();
		ImGui::EndChild();

		ImGui::SameLine();

		// Right: properties of the selected tile
		ImGui::BeginChild("TileProperties", ImVec2(0, 0), ImGuiChildFlags_Borders);
		if (ImGui::CollapsingHeader("Tilesets"))
			DrawTilesetPanel();
		DrawTileProperties();
		ImGui::EndChild();

		ModalPopup("Remove Tile", m_confirmRemove, ImGuiHelpers::POPUP_WINDOW_SIZE, [&] {
			ImGui::Text("Remove tile %u (\"%s\")?", static_cast<unsigned>(m_selectedId), m_draft.name.c_str());
			ImGui::TextWrapped("Rooms that use this tile will fail to load until the tile is added back.");
			ImGui::Separator();

			if (ImGui::Button("Remove"))
			{
				RemoveSelectedTile();
				m_confirmRemove = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				m_confirmRemove = false;
				ImGui::CloseCurrentPopup();
			}
		});

		ModalPopup("Unsaved Tile Changes", m_discardPrompt, ImGuiHelpers::POPUP_WINDOW_SIZE, [&] {
			ImGui::TextUnformatted("This tile has unsaved changes.");
			ImGui::TextUnformatted("Discard them and switch tiles?");
			ImGui::Separator();

			if (ImGui::Button("Discard"))
			{
				LoadDraft(m_pendingSelectId);
				m_discardPrompt = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				m_discardPrompt = false;
				ImGui::CloseCurrentPopup();
			}
		});
	}

	void TilesEditor::DrawTileGrid()
	{
		if (ImGui::Button("+ Add Tile"))
			AddTile();

		const auto& tiles = m_tileDatabase->All();
		ImGui::SameLine();
		ImGui::TextDisabled("%zu tiles", tiles.size());
		ImGui::Separator();

		if (tiles.empty())
		{
			ImGui::TextDisabled("No tiles yet. Click \"+ Add Tile\".");
			return;
		}

		constexpr int kColumns = 16;
		constexpr float kGap = 2.0f;

		// Cell size is derived from the panel width so there are always 16 columns.
		const float avail = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize;
		const float cell = (std::max) (12.0f, std::floor((avail - kGap * (kColumns - 1)) / kColumns));

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(kGap, kGap));
		ImDrawList* dl = ImGui::GetWindowDrawList();

		const int count = static_cast<int>(tiles.size());
		int index = 0;
		TileID clicked = 0;

		const TilesetInfo& tileset = m_tileDatabase->GetTileset(m_selectedTileset)->info;

		for (const auto& [id, def] : tiles) // std::map: ascending by ID
		{
			ImGui::PushID(static_cast<int>(id));

			if (ImGui::InvisibleButton("##tile", ImVec2(cell, cell)))
				clicked = id; // applied after the loop

			const ImVec2 a = ImGui::GetItemRectMin();
			const ImVec2 b = ImGui::GetItemRectMax();

			DrawTileImage(dl, a, b, &def, *m_tileDatabase, id, 255);

			char label[8];
			std::snprintf(label, sizeof(label), "%u", static_cast<unsigned>(id));
			const ImVec2 ts = ImGui::CalcTextSize(label);
			if (ts.x <= cell)
				dl->AddText(ImVec2(a.x + (cell - ts.x) * 0.5f, a.y + (cell - ts.y) * 0.5f),
							IM_COL32(255, 255, 255, 230), label);

			if (id == m_selectedId)
				dl->AddRect(a, b, IM_COL32(255, 220, 60, 255), 0.0f, 0, 2.0f);
			else if (ImGui::IsItemHovered())
				dl->AddRect(a, b, IM_COL32(255, 255, 255, 160));

			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%u: %s", static_cast<unsigned>(id), def.name.c_str());

			if (m_scrollToId == id)
			{
				ImGui::SetScrollHereY();
				m_scrollToId = 0;
			}

			ImGui::PopID();

			if ((index % kColumns) != kColumns - 1 && index + 1 < count)
				ImGui::SameLine();
			++index;
		}

		ImGui::PopStyleVar();

		if (clicked != 0)
			SelectTile(clicked);
	}

	void TilesEditor::DrawTileProperties()
	{
		if (m_selectedId == 0)
		{
			ImGui::TextDisabled("Select a tile on the left, or add a new one.");
			StatusText();
			return;
		}

		ImGui::Text("Tile ID: %u", static_cast<unsigned>(m_selectedId)); // IDs are fixed: rooms refer to them
		if (m_draftDirty)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(unsaved changes)");
		}
		ImGui::Separator();

		bool changed = false;
		changed |= InputString("Name", m_draft.name);
		changed |= SpritePicker(m_draft);
		changed |= ImGui::Checkbox("Blocks movement", &m_draft.blocksMovement);
		changed |= ImGui::Checkbox("Blocks projectiles", &m_draft.blocksProjectiles);
		changed |= ImGui::Checkbox("Destructible", &m_draft.destructible);
		changed |= ImGui::InputInt("Contact damage", &m_draft.contactDamage);
		m_draft.contactDamage = (std::max) (0, m_draft.contactDamage);

		if (changed)
			m_draftDirty = true;

		ImGui::Separator();

		ImGui::BeginDisabled(!m_draftDirty);
		if (ImGui::Button("Save"))
			SaveSelectedTile();
		ImGui::SameLine();
		if (ImGui::Button("Revert"))
			LoadDraft(m_selectedId);
		ImGui::EndDisabled();

		ImGui::SameLine();
		if (ImGui::Button("Remove Tile"))
			m_confirmRemove = true;

		ImGui::Separator();
		StatusText();
	}

	void TilesEditor::DrawTilesetPanel()
	{
		const auto& sets = m_tileDatabase->Tilesets();
		if (sets.empty())
			return;

		if (!sets.contains(m_selectedTileset))
		{
			m_selectedTileset = sets.begin()->first;
			m_tilesetDraftLoaded = false;
		}
		if (!m_tilesetDraftLoaded)
		{
			m_tilesetDraft = sets.at(m_selectedTileset).info;
			m_tilesetDraftLoaded = true;
		}

		const std::string preview = std::to_string(m_selectedTileset) + ": " + sets.at(m_selectedTileset).info.name;
		ImGui::SetNextItemWidth(-90.0f);
		if (ImGui::BeginCombo("##tileset", preview.c_str()))
		{
			for (const auto& [id, entry] : sets)
			{
				const std::string l = std::to_string(id) + ": " + entry.info.name;
				if (ImGui::Selectable(l.c_str(), id == m_selectedTileset))
				{
					m_selectedTileset = id;
					m_tilesetDraftLoaded = false;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("+ New"))
		{
			TilesetInfo info;
			info.name = "Tileset " + std::to_string(m_tileDatabase->NextFreeTilesetID());
			const TilesetID id = m_tileDatabase->NextFreeTilesetID();
			if (m_tileDatabase->RegisterTileset(id, info) && m_tileDatabase->SaveToFile())
			{
				m_selectedTileset = id;
				m_tilesetDraftLoaded = false;
				m_status = "Added tileset " + std::to_string(id) + ", set its image path below";
			}
		}

		InputString("Name##ts", m_tilesetDraft.name);
		InputString("Image##ts", m_tilesetDraft.path);   // relative to Assets
		ImGui::InputInt("Tile size##ts", &m_tilesetDraft.tileSize);
		ImGui::InputInt("Spacing##ts", &m_tilesetDraft.spacing);
		ImGui::InputInt("Margin##ts", &m_tilesetDraft.margin);

		if (ImGui::Button("Apply"))
		{
			if (m_tileDatabase->RegisterTileset(m_selectedTileset, m_tilesetDraft) && m_tileDatabase->SaveToFile())
				m_status = "Saved tileset " + std::to_string(m_selectedTileset);
			else
				m_status = "Invalid tileset settings (check path, padding and cell size)";
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove"))
		{
			m_status = m_tileDatabase->UnregisterTileset(m_selectedTileset) && m_tileDatabase->SaveToFile()
				? "Removed tileset"
				: "Can't remove: tiles still use it, or it's the last tileset";
			m_tilesetDraftLoaded = false;
		}
		ImGui::Separator();
	}

	bool TilesEditor::SpritePicker(TileDef& def)
	{
		bool changed = false;
		const auto& sets = m_tileDatabase->Tilesets();

		auto label = [](TilesetID id, const TilesetInfo& i) { return std::to_string(id) + ": " + i.name; };
		const auto curIt = sets.find(def.tileset);
		const std::string preview = curIt != sets.end()
			? label(curIt->first, curIt->second.info)
			: "Missing (" + std::to_string(def.tileset) + ")";

		if (ImGui::BeginCombo("Tileset", preview.c_str()))
		{
			for (const auto& [id, entry] : sets)
			{
				if (ImGui::Selectable(label(id, entry.info).c_str(), id == def.tileset) && id != def.tileset)
				{
					def.tileset = id;
					def.spriteCol = def.spriteRow = -1;   // col/row mean nothing in a different image
					changed = true;
				}
			}
			ImGui::EndCombo();
		}

		const TilesetEntry* entry = m_tileDatabase->GetTileset(def.tileset);

		ImGui::TextUnformatted("Sprite");
		const SpriteRef cur = m_tileDatabase->GetSprite(def);
		
		if (cur.valid)
			ImGui::Image(cur.texture, ImVec2(48, 48), cur.uv0, cur.uv1);
		else
			ImGui::Button(def.spriteCol >= 0 ? "missing" : "none", ImVec2(48, 48));

		ImGui::SameLine();
		if (ImGui::Button("Pick..."))
			ImGui::OpenPopup("PickSprite");
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
		{
			def.spriteCol = def.spriteRow = -1;
			changed = true;
		}

		if (ImGui::BeginPopup("PickSprite"))
		{
			const SpriteRef whole = entry ? entry->sprites.WholeTileset() : SpriteRef{};
			if (!entry || !whole.valid)
			{
				ImGui::TextUnformatted("Tileset not loaded");
			}
			else
			{
				const TilesetInfo& info = entry->info;
				const TileSprites& sprites = entry->sprites;
				static float scale = 2.0f;
				ImGui::SetNextItemWidth(120.0f);
				ImGui::SliderFloat("Zoom", &scale, 1.0f, 4.0f, "%.0fx");

				const ImVec2 size(sprites.TilesetW() * scale, sprites.TilesetH() * scale);
				const float stride = info.CellSize() * scale;   // distance from one tile origin to the next
				const float margin = info.margin * scale;       // image edge to the first tile
				const float art = info.tileSize * scale;        // the tile's own size

				// How many whole tiles fit (the last tile has no trailing gap)
				const int cols = (int) ((sprites.TilesetW() - info.margin + info.spacing) / info.CellSize());
				const int rows = (int) ((sprites.TilesetH() - info.margin + info.spacing) / info.CellSize());

				const ImVec2 o = ImGui::GetCursorScreenPos();

				ImGui::InvisibleButton("##sheet", size);
				const bool hovered = ImGui::IsItemHovered();
				const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

				ImDrawList* dl = ImGui::GetWindowDrawList();
				dl->AddImage(whole.texture, o, ImVec2(o.x + size.x, o.y + size.y), whole.uv0, whole.uv1);

				// One box per tile, drawn around the art itself
				auto outline = [&](int c, int r, ImU32 color, float thickness) {
					const ImVec2 a(o.x + margin + c * stride, o.y + margin + r * stride);
					dl->AddRect(a, ImVec2(a.x + art, a.y + art), color, 0.0f, 0, thickness);
				};

				for (int r = 0; r < rows; ++r)
					for (int c = 0; c < cols; ++c)
						outline(c, r, IM_COL32(255, 255, 255, 40), 1.0f);

				if (def.spriteCol >= 0 && def.spriteCol < cols && def.spriteRow >= 0 && def.spriteRow < rows)
					outline(def.spriteCol, def.spriteRow, IM_COL32(255, 220, 60, 255), 2.0f);

				if (hovered)
				{
					const ImVec2 m = ImGui::GetMousePos();
					const int c = (int) std::floor((m.x - o.x - margin) / stride);
					const int r = (int) std::floor((m.y - o.y - margin) / stride);
					if (c >= 0 && r >= 0 && c < cols && r < rows)
					{
						outline(c, r, IM_COL32(255, 255, 255, 200), 2.0f);
						ImGui::SetTooltip("(%d, %d)", c, r);
						if (clicked)
						{
							def.spriteCol = c;
							def.spriteRow = r;
							changed = true;
							ImGui::CloseCurrentPopup();
						}
					}
				}
			}
			ImGui::EndPopup();
		}
		return changed;
	}

	void TilesEditor::SelectTile(TileID id)
	{
		if (id == m_selectedId)
			return;

		if (m_draftDirty)
		{
			m_pendingSelectId = id;
			m_discardPrompt = true;
			return;
		}
		LoadDraft(id);
	}

	void TilesEditor::LoadDraft(TileID id)
	{
		const TileDef* def = m_tileDatabase->Get(id);
		if (!def)
		{
			m_selectedId = 0;
			m_draft = TileDef{};
		}
		else
		{
			m_selectedId = id;
			m_draft = *def;
		}
		m_draftDirty = false;
		m_status.clear();
	}

	void TilesEditor::AddTile()
	{
		const TileID id = m_tileDatabase->NextFreeID();
		if (id == 0)
		{
			m_status = "No free tile IDs left";
			return;
		}

		TileDef def;
		def.name = "Tile " + std::to_string(id);
		def.tileset = m_selectedTileset;
		m_tileDatabase->Register(id, def);

		const bool saved = m_tileDatabase->SaveToFile();

		SelectTile(id); // may open the discard prompt if another tile has unsaved edits
		m_scrollToId = id;
		m_status = saved ? "Added tile " + std::to_string(id)
			: "Added tile " + std::to_string(id) + ", but saving to disk failed";
	}

	void TilesEditor::RemoveSelectedTile()
	{
		if (m_selectedId == 0)
			return;

		const TileID removed = m_selectedId;

		// Choose the neighbor to select afterwards, before the iterators go stale.
		TileID next = 0;
		const auto& all = m_tileDatabase->All();
		const auto it = all.find(removed);
		if (it != all.end())
		{
			const auto after = std::next(it);
			if (after != all.end())
				next = after->first;
			else if (it != all.begin())
				next = std::prev(it)->first;
		}

		m_tileDatabase->Unregister(removed);
		const bool saved = m_tileDatabase->SaveToFile();

		m_selectedId = 0;
		m_draftDirty = false;
		if (next != 0)
			LoadDraft(next);

		m_status = saved ? "Removed tile " + std::to_string(removed)
			: "Removed tile " + std::to_string(removed) + ", but saving to disk failed";
	}

	bool TilesEditor::SaveSelectedTile()
	{
		if (m_selectedId == 0)
			return false;

		if (m_draft.name.empty())
		{
			m_status = "Name can't be empty";
			return false;
		}

		m_tileDatabase->Register(m_selectedId, m_draft);
		if (!m_tileDatabase->SaveToFile())
		{
			m_status = "Save failed"; // draft stays dirty so you can retry
			return false;
		}

		m_draftDirty = false;
		m_status = "Saved tile " + std::to_string(m_selectedId);
		return true;
	}

	void TilesEditor::StatusText()
	{
		if (!m_status.empty())
			ImGui::TextWrapped("%s", m_status.c_str());
	}
}