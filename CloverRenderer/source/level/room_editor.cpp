#include "level/room_editor.hpp"
#include "core/imgui_helpers.hpp"
#include "level/room_serialization.hpp"

#include <core/engine.hpp>
#include <cmath>
#include <level/backdrop.hpp>
#include <rendering/renderer.hpp>

namespace clvr
{
	RoomEditor::RoomEditor() : EditorWindow("Room Editor", ImGuiWindowFlags_MenuBar)
	{
		title = Title();
		priority = 25;

		// init variables
		m_visualLayerVisible.assign(CLVR_MAX_VISUAL_LAYERS, true);

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

		m_backdropDatabase = Engine.GetResourceManager()->Load<BackdropDatabase>(
			Engine.GetResourceManager()->GetPath(ResourceManager::Directory::Assets, "backdrops/backdrops.json").c_str());

		if (!m_backdropDatabase)
		{
			// create a new backdrop database if it doesn't exist
			BackdropDatabase newBackdropDatabase(Engine.GetResourceManager()->GetPath(ResourceManager::Directory::Assets, "backdrops/backdrops.json").c_str());
			newBackdropDatabase.SaveToFile();
			m_backdropDatabase = Engine.GetResourceManager()->Load<BackdropDatabase>(
				Engine.GetResourceManager()->GetPath(ResourceManager::Directory::Assets, "backdrops/backdrops.json").c_str());
		}

		auto& renderer = Engine.GetECS()->GetSystem<Renderer>();
		auto* resources = Engine.GetResourceManager();

		m_backdropDatabase->Sprites().Init(*renderer.GetTextureAtlas(), *m_backdropDatabase,
										   [resources](const std::string& relative) {
			return resources->GetPath(ResourceManager::Directory::Assets, relative);
		});
	}

	RoomEditor::~RoomEditor()
	{}

	void RoomEditor::Update(float dt)
	{
		m_backdropDatabase->Sprites().Refresh();
	}

	void RoomEditor::Render()
	{}

	void RoomEditor::Inspect(float dt)
	{
		DrawWindow();
	}

	void RoomEditor::OnPlayStart()
	{}

	void RoomEditor::OnPlayStop()
	{}

	json RoomEditor::Save()
	{
		return json();
	}

	void RoomEditor::Load(const json&)
	{}

	namespace
	{
		bool IsValidRoomName(const std::string& s)
		{
			if (s.empty() || s.size() > 64)
				return false;
			return std::all_of(s.begin(), s.end(), [](unsigned char c) {
				return std::isalnum(c) || c == '_' || c == '-';
			});
		}
	}

	std::filesystem::path RoomEditor::RoomPath(const std::string& name) const
	{
		return std::filesystem::path(Engine.GetResourceManager()->GetPath(ResourceManager::Directory::Assets, "rooms")) / (name + ".json");
	}

	bool RoomEditor::CreateRoom(const std::string& name)
	{
		CancelStroke();
		m_activeLayer.kind = LayerKind::Visual;
		m_activeLayer.index = 0;

		if (!IsValidRoomName(name)) { m_status = "Use letters, digits, _ and - only"; return false; }

		const auto path = RoomPath(name);
		if (std::filesystem::exists(path)) { m_status = "That room already exists"; return false; }

		RoomData room;
		if (!SaveRoomToFile(path, room)) { m_status = "Could not write " + path.string(); return false; }

		m_currentRoom = std::move(room);
		
		m_visualLayerVisible.assign(m_currentRoom.VisualLayerCount(), true);

		m_currentRoomName = name;
		m_dirty = false;
		return true;
	}

	bool RoomEditor::SaveRoom(const std::string& name)
	{
		if (!IsValidRoomName(name)) { m_status = "Invalid room name"; return false; }

		if (!SaveRoomToFile(RoomPath(name), m_currentRoom)) { m_status = "Save failed"; return false; }

		m_currentRoomName = name;
		m_dirty = false;
		m_status = "Saved " + name;
		return true;
	}

	bool RoomEditor::LoadRoom(const std::string& name)
	{
		CancelStroke();
		m_activeLayer.kind = LayerKind::Visual;
		m_activeLayer.index = 0;

		if (!m_tileDatabase) 
		{ 
			m_status = "Tile database not loaded"; 
			return false; 
		}

		std::string error;
		if (!LoadRoomFromFile(RoomPath(name), *m_tileDatabase, m_currentRoom, error))
		{
			m_status = error; // m_currentRoom is untouched on failure
			return false;
		}

		m_visualLayerVisible.assign(m_currentRoom.VisualLayerCount(), true);

		m_currentRoomName = name;
		m_dirty = false;
		return true;
	}

	void RoomEditor::DrawContents()
	{
		// create a room editor window with a menu bar and a button to create a new room
		MenuBar();

		// If no room is open, show a message instead of the editor.
		if (m_currentRoomName.empty())
		{
			ImVec2 size = ImGui::GetWindowSize();
			std::string message = "No room open. Use File > New Room or File > Open Room to get started.";
			ImGui::SetCursorPosY(size.y / 2 - ImGui::GetTextLineHeight() / 2);
			ImGui::SetCursorPosX(size.x / 2 - ImGui::CalcTextSize(message.c_str()).x / 2);
			ImGui::TextUnformatted(message.c_str());
			return;
		}

		ToolBar();
		ImGui::Separator();

		// Left panel: layers (the tile palette will go below it later)
		ImGui::BeginChild("LeftPanel", ImVec2(220.0f, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
		RoomProperties();
		LayerPanel();
		ImGui::Separator();
		TilePalette();
		ImGui::EndChild();

		ImGui::SameLine();

		DrawCanvas();
	}

	void RoomEditor::OnOpen()
	{}

	void RoomEditor::OnClose()
	{
		CancelStroke();
	}

	Grid<TileID>* RoomEditor::ActiveTileGrid()
	{
		switch (m_activeLayer.kind)
		{
			case LayerKind::Visual:
				if (m_activeLayer.index >= 0 && m_activeLayer.index < m_currentRoom.VisualLayerCount())
					return &m_currentRoom.visualLayers[m_activeLayer.index];
				return nullptr;
			case LayerKind::Gameplay:
				return &m_currentRoom.gameplayLayer;
			case LayerKind::Spawn:
				return nullptr; // handled by the spawn tool, different ID type
		}
		return nullptr;
	}

	void RoomEditor::MenuBar()
	{
		// Runs the action chosen in the discard prompt one frame later, so the
		// next popup doesn't open on the same frame this one closes.
		if (m_discardConfirmed)
		{
			m_discardConfirmed = false;
			auto action = std::move(m_pendingAction);
			m_pendingAction = nullptr;
			if (action)
				action();
		}

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Room"))
				{
					RunUnlessDirty([this] {
						SetNameBuffer("");
						m_status.clear();
						m_newRoomCreator = true;
					});
				}
				if (ImGui::MenuItem("Open Room..."))
				{
					RunUnlessDirty([this] {
						RefreshRoomList();
						m_status.clear();
						m_openRoomLoader = true;
					});
				}

				ImGui::Separator();

				const bool hasRoom = !m_currentRoomName.empty();
				if (ImGui::MenuItem("Save", "Ctrl+S", false, hasRoom))
					SaveRoom(m_currentRoomName);

				if (ImGui::MenuItem("Save As..."))
				{
					RefreshRoomList();
					SetNameBuffer(m_currentRoomName);
					m_status.clear();
					m_confirmOverwrite = false;
					m_openRoomSaver = true;
				}
				ImGui::EndMenu();
			}

			const std::string label =
				(m_currentRoomName.empty() ? "(no room open)" : m_currentRoomName) + (m_dirty ? " *" : "");
			ImGui::TextDisabled("%s", label.c_str());

			ImGui::EndMenuBar();
		}

		// Ctrl+S, only while this window (or its children) has focus
		if (!m_currentRoomName.empty() &&
			ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
			ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
		{
			SaveRoom(m_currentRoomName);
		}

		// ---- New Room ----
		ModalPopup("New Room", m_newRoomCreator, ImGuiHelpers::POPUP_WINDOW_SIZE, [&] {
			ImGui::TextUnformatted("Enter a name for the new room:");
			ImGui::Separator();
			ImGui::InputText("Room Name", m_roomNameBuffer, sizeof(m_roomNameBuffer));
			StatusText();
			ImGui::Separator();

			if (ImGui::Button("Create") && CreateRoom(m_roomNameBuffer))
			{
				m_newRoomCreator = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				m_newRoomCreator = false;
				ImGui::CloseCurrentPopup();
			}
		});

		// ---- Save As ----
		ModalPopup("Save Room As", m_openRoomSaver, ImGuiHelpers::POPUP_WINDOW_SIZE, [&] {
			if (m_confirmOverwrite)
			{
				ImGui::Text("\"%s\" already exists. Overwrite it?", m_roomNameBuffer);
				ImGui::Separator();

				if (ImGui::Button("Overwrite"))
				{
					const bool saved = SaveRoom(m_roomNameBuffer);
					m_confirmOverwrite = false;
					if (saved)
					{
						m_openRoomSaver = false;
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Back"))
					m_confirmOverwrite = false;
				return;
			}

			ImGui::TextUnformatted("Existing rooms (click to copy the name):");
			for (const FileInfo& roomFile : m_roomFiles)
				if (ImGui::Selectable(roomFile.name.c_str()))
					SetNameBuffer(roomFile.name); // fills the box, doesn't save

			ImGui::Separator();
			ImGui::InputText("Room Name", m_roomNameBuffer, sizeof(m_roomNameBuffer));
			StatusText();
			ImGui::Separator();

			if (ImGui::Button("Save"))
			{
				const std::string name = m_roomNameBuffer;
				if (IsValidRoomName(name) && RoomExists(name) && name != m_currentRoomName)
				{
					m_confirmOverwrite = true;
				}
				else if (SaveRoom(name))
				{
					m_openRoomSaver = false;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				m_openRoomSaver = false;
				ImGui::CloseCurrentPopup();
			}
		});

		// ---- Open ----
		ModalPopup("Open Room", m_openRoomLoader, ImGuiHelpers::POPUP_WINDOW_SIZE, [&] {
			ImGui::TextUnformatted("Which room would you like to open?");
			ImGui::Separator();

			for (const FileInfo& roomFile : m_roomFiles)
			{
				if (ImGui::Selectable(roomFile.name.c_str()) && LoadRoom(roomFile.name))
				{
					m_openRoomLoader = false;
					ImGui::CloseCurrentPopup();
				}
			}

			StatusText();
			ImGui::Separator();
			if (ImGui::Button("Cancel"))
			{
				m_openRoomLoader = false;
				ImGui::CloseCurrentPopup();
			}
		});

		// ---- Unsaved changes ----
		ModalPopup("Unsaved Changes", m_discardPrompt, ImGuiHelpers::POPUP_WINDOW_SIZE, [&] {
			ImGui::TextUnformatted("The current room has unsaved changes.");
			ImGui::TextUnformatted("Discard them and continue?");
			ImGui::Separator();

			if (ImGui::Button("Discard"))
			{
				m_discardConfirmed = true; // action runs next frame
				m_discardPrompt = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				m_pendingAction = nullptr;
				m_discardPrompt = false;
				ImGui::CloseCurrentPopup();
			}
		});
	}

	namespace
	{
		ImU32 IdColor(unsigned id, int alpha)
		{
			return IM_COL32((id * 37) % 200 + 40, (id * 59) % 200 + 40, (id * 83) % 200 + 40, alpha);
		}
	}

	void RoomEditor::DrawCanvas() 
	{
		const float cell = 16.0f * m_zoom;
		const int w = m_currentRoom.width;
		const int h = m_currentRoom.height;
		const ImVec2 roomSize(w * cell, h * cell);

		// The canvas covers the room grid plus the backdrop's wall frame around it.
		const BackdropDef* backdrop = m_backdropDatabase
			? m_backdropDatabase->Get(m_currentRoom.backdropId) : nullptr;
		const int margin = backdrop ? backdrop->wallThickness : 0;
		const int extentW = backdrop ? (std::max) (w, backdrop->widthTiles - margin) : w;  // cells right of the grid origin
		const int extentH = backdrop ? (std::max) (h, backdrop->heightTiles - margin) : h;
		const ImVec2 canvasSize((margin + extentW) * cell, (margin + extentH) * cell);

		ImGui::BeginChild("Canvas", ImVec2(0, 0), ImGuiChildFlags_Borders,
						  ImGuiWindowFlags_HorizontalScrollbar);

		const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos(); // top-left of the whole canvas
		const ImVec2 origin(canvasOrigin.x + margin * cell,      // top-left of the room grid
							canvasOrigin.y + margin * cell);

		ImGui::InvisibleButton("##canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft);
		const bool hovered = ImGui::IsItemHovered();
		CanvasInput(origin, cell);

		ImDrawList* dl = ImGui::GetWindowDrawList();
		auto cellMin = [&](int x, int y) { return ImVec2(origin.x + x * cell, origin.y + y * cell); };
		auto cellMax = [&](int x, int y) { return ImVec2(origin.x + (x + 1) * cell, origin.y + (y + 1) * cell); };

		const TilesetInfo& tileset = m_tileDatabase->Tileset();
		const TileSprites& sprites = m_tileDatabase->Sprites();

		// Background for the whole canvas, then the backdrop on top of it
		dl->AddRectFilled(canvasOrigin,
						  ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y),
						  IM_COL32(25, 25, 30, 255));

		if (backdrop && m_showBackdrop)
		{
			// The backdrop image starts at the canvas corner and spans widthTiles x heightTiles
			const ImVec2 bMin = canvasOrigin;
			const ImVec2 bMax(canvasOrigin.x + backdrop->widthTiles * cell,
							  canvasOrigin.y + backdrop->heightTiles * cell);

			const SpriteRef bs = m_backdropDatabase->Sprites().Get(m_currentRoom.backdropId);
			if (bs.valid)
			{
				dl->AddImage(bs.texture, bMin, bMax, bs.uv0, bs.uv1);
			}
			else
			{
				dl->AddRectFilled(bMin, bMax, IM_COL32(255, 0, 255, 60)); // magenta: image didn't resolve
				dl->AddText(ImVec2(bMin.x + 8, bMin.y + 8), IM_COL32(255, 255, 255, 220), "backdrop image missing");
			}
		}

		// Visual layers, bottom to top. The active one is full strength, the rest dimmed.
		for (int i = 0; i < m_currentRoom.VisualLayerCount(); ++i)
		{
			if (i < static_cast<int>(m_visualLayerVisible.size()) && !m_visualLayerVisible[i])
				continue;

			const bool active = m_activeLayer.kind == LayerKind::Visual && m_activeLayer.index == i;
			const int alpha = active ? 255 : 90;
			const Grid<TileID>& layer = m_currentRoom.visualLayers[i];

			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x)
				{
					const TileID id = layer.At(x, y);
					if (id != 0)
						DrawTileImage(dl, cellMin(x, y), cellMax(x, y),
									  m_tileDatabase->Get(id), sprites, tileset, id, alpha);
				}
		}

		// Gameplay layer: inset squares so they read differently from the art
		if (m_showGameplay)
		{
			const bool active = m_activeLayer.kind == LayerKind::Gameplay;
			const int alpha = active ? 220 : 100;
			const float inset = cell * 0.2f;

			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x)
				{
					const TileID id = m_currentRoom.gameplayLayer.At(x, y);
					if (id == 0)
						continue;
					const ImVec2 a = cellMin(x, y), b = cellMax(x, y);
					dl->AddRectFilled(ImVec2(a.x + inset, a.y + inset), ImVec2(b.x - inset, b.y - inset),
									  IdColor(id + 100, alpha));
				}
		}

		// Spawn layer: circles with the ID drawn on top
		if (m_showSpawns)
		{
			const bool active = m_activeLayer.kind == LayerKind::Spawn;
			const int alpha = active ? 255 : 120;

			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x)
				{
					const auto id = m_currentRoom.spawnLayer.At(x, y);
					if (id == 0)
						continue;
					const ImVec2 a = cellMin(x, y);
					const ImVec2 center(a.x + cell * 0.5f, a.y + cell * 0.5f);
					dl->AddCircleFilled(center, cell * 0.3f, IdColor(static_cast<unsigned>(id) + 200, alpha));
					if (cell >= 32.0f)
					{
						const std::string text = std::to_string(static_cast<unsigned>(id));
						const ImVec2 ts = ImGui::CalcTextSize(text.c_str());
						dl->AddText(ImVec2(center.x - ts.x * 0.5f, center.y - ts.y * 0.5f),
									IM_COL32(255, 255, 255, alpha), text.c_str());
					}
				}
		}

		if (m_rectDragging)
		{
			const ImVec2 mouse = ImGui::GetMousePos();
			const int cx = std::clamp(static_cast<int>(std::floor((mouse.x - origin.x) / cell)), 0, w - 1);
			const int cy = std::clamp(static_cast<int>(std::floor((mouse.y - origin.y) / cell)), 0, h - 1);

			const int x0 = (std::min) (m_rectX0, cx), x1 = (std::max) (m_rectX0, cx);
			const int y0 = (std::min) (m_rectY0, cy), y1 = (std::max) (m_rectY0, cy);

			dl->AddRectFilled(cellMin(x0, y0), cellMax(x1, y1), IM_COL32(255, 220, 60, 50));
			dl->AddRect(cellMin(x0, y0), cellMax(x1, y1), IM_COL32(255, 220, 60, 255), 0.0f, 0, 2.0f);
		}

		// Grid lines and room border
		const ImU32 gridColor = IM_COL32(255, 255, 255, 30);
		for (int x = 0; x <= w; ++x)
			dl->AddLine(ImVec2(origin.x + x * cell, origin.y),
						ImVec2(origin.x + x * cell, origin.y + roomSize.y), gridColor);
		for (int y = 0; y <= h; ++y)
			dl->AddLine(ImVec2(origin.x, origin.y + y * cell),
						ImVec2(origin.x + roomSize.x, origin.y + y * cell), gridColor);
		dl->AddRect(origin, ImVec2(origin.x + roomSize.x, origin.y + roomSize.y),
					IM_COL32(255, 255, 255, 120));

		// Hovered cell: screen position -> grid cell
		m_hoverX = m_hoverY = -1;
		if (hovered)
		{
			const ImVec2 mouse = ImGui::GetMousePos();
			const int cx = static_cast<int>(std::floor((mouse.x - origin.x) / cell));
			const int cy = static_cast<int>(std::floor((mouse.y - origin.y) / cell));

			if (cx >= 0 && cy >= 0 && cx < w && cy < h)
			{
				m_hoverX = cx;
				m_hoverY = cy;
				dl->AddRect(cellMin(cx, cy), cellMax(cx, cy), IM_COL32(255, 220, 60, 255), 0.0f, 0, 2.0f);
				ImGui::SetTooltip("(%d, %d)", cx, cy);
			}
		}

		ImGui::EndChild();
	}

	void RoomEditor::LayerPanel()
	{
		// Visual layers, topmost first
		for (int i = m_currentRoom.VisualLayerCount() - 1; i >= 0; --i)
		{
			ImGui::PushID(i);

			bool visible = m_visualLayerVisible[i];
			if (ImGui::Checkbox("##vis", &visible))
				m_visualLayerVisible[i] = visible;
			ImGui::SameLine();

			const std::string label = "Visual " + std::to_string(i);
			const bool selected = m_activeLayer.kind == LayerKind::Visual && m_activeLayer.index == i;
			if (ImGui::Selectable(label.c_str(), selected))
				m_activeLayer = { LayerKind::Visual, i };

			ImGui::PopID();
		}

		ImGui::Separator();

		ImGui::Checkbox("##gp", &m_showGameplay);
		ImGui::SameLine();
		if (ImGui::Selectable("Gameplay", m_activeLayer.kind == LayerKind::Gameplay))
			m_activeLayer = { LayerKind::Gameplay, 0 };

		ImGui::Checkbox("##sp", &m_showSpawns);
		ImGui::SameLine();
		if (ImGui::Selectable("Spawns", m_activeLayer.kind == LayerKind::Spawn))
			m_activeLayer = { LayerKind::Spawn, 0 };

		if (ImGui::Button("Add Visual Layer") && m_currentRoom.AddVisualLayer())
		{
			m_visualLayerVisible.push_back(true);
			m_activeLayer = { LayerKind::Visual, m_currentRoom.VisualLayerCount() - 1 };
			m_dirty = true;
		}

		const bool onVisual = m_activeLayer.kind == LayerKind::Visual;
		const bool canRemove = onVisual && m_currentRoom.VisualLayerCount() > 1;

		ImGui::BeginDisabled(!canRemove);
		if (ImGui::Button("Remove Visual Layer"))
		{
			const int index = m_activeLayer.index;
			if (m_currentRoom.RemoveVisualLayer(index))
			{
				if (index < static_cast<int>(m_visualLayerVisible.size()))
					m_visualLayerVisible.erase(m_visualLayerVisible.begin() + index);

				m_activeLayer = { LayerKind::Visual, (std::max) (0, index - 1) };
				m_dirty = true;
			}
		}
		ImGui::EndDisabled();
	}

	void RoomEditor::RefreshRoomList()
	{
		m_roomFiles = Engine.GetResourceManager()->GetFilesInDirectory(
			ResourceManager::Directory::Assets, "rooms", ".json");

		for (FileInfo& file : m_roomFiles)
			file.name = std::filesystem::path(file.name).stem().string();
	}

	void RoomEditor::SetNameBuffer(const std::string& s)
	{
		std::snprintf(m_roomNameBuffer, sizeof(m_roomNameBuffer), "%s", s.c_str());
	}

	bool RoomEditor::RoomExists(const std::string& name) const
	{
		return std::filesystem::exists(RoomPath(name));
	}

	void RoomEditor::StatusText()
	{
		if (!m_status.empty())
			ImGui::TextWrapped("%s", m_status.c_str());
	}

	void RoomEditor::RoomProperties()
	{
		ImGui::SeparatorText("Room");
		ImGui::TextUnformatted("Backdrop");
		BackdropPicker();
		ImGui::Checkbox("Show backdrop", &m_showBackdrop);
	}

	void RoomEditor::SetBackdrop(BackdropID id)
	{
		if (m_currentRoom.backdropId == id)
			return;
		m_currentRoom.backdropId = id;
		m_dirty = true;
	}

	void RoomEditor::BackdropPicker()
	{
		if (!m_backdropDatabase)
		{
			ImGui::TextDisabled("Backdrop database not loaded");
			return;
		}

		auto label = [](BackdropID id, const BackdropDef& def) {
			return def.name.empty() ? "Backdrop " + std::to_string(id) : def.name;
		};

		const BackdropID currentId = m_currentRoom.backdropId;
		const BackdropDef* current = m_backdropDatabase->Get(currentId);

		std::string preview;
		if (currentId == 0)
			preview = "None";
		else if (current)
			preview = label(currentId, *current);
		else
			preview = "Missing (" + std::to_string(currentId) + ")";

		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::BeginCombo("##backdrop", preview.c_str()))
		{
			if (ImGui::Selectable("None", currentId == 0))
				SetBackdrop(0);

			for (const auto& [id, def] : m_backdropDatabase->All())
			{
				ImGui::PushID(static_cast<int>(id));

				const bool selected = (id == currentId);
				if (ImGui::Selectable(label(id, def).c_str(), selected))
					SetBackdrop(id);
				if (selected)
					ImGui::SetItemDefaultFocus();

				ImGui::PopID();
			}

			if (m_backdropDatabase->All().empty())
				ImGui::TextDisabled("No backdrops defined yet");

			ImGui::EndCombo();
		}

		// The backdrop must fit the room's playable area
		if (current &&
			(current->PlayableWidth() != m_currentRoom.width ||
			 current->PlayableHeight() != m_currentRoom.height))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 80, 255));
			ImGui::TextWrapped("Size mismatch: this backdrop's playable area is %dx%d, the room is %dx%d.",
							   current->PlayableWidth(), current->PlayableHeight(),
							   m_currentRoom.width, m_currentRoom.height);
			ImGui::PopStyleColor();
		}
		else if (currentId != 0 && !current)
		{
			ImGui::TextWrapped("This backdrop no longer exists. Pick another one.");
		}
	}

	// Runs the action now, or after the user confirms discarding unsaved work.
	void RoomEditor::RunUnlessDirty(std::function<void()> action)
	{
		if (!m_dirty)
		{
			action();
			return;
		}
		m_pendingAction = std::move(action);
		m_discardPrompt = true;
	}

	void RoomEditor::ToolBar()
	{
		struct ToolInfo { PaintTool tool; const char* label; const char* tip; ImGuiKey key; };
		static const ToolInfo tools[] = {
			{ PaintTool::Draw,      "Draw",  "Paint the brush tile (B)",           ImGuiKey_B },
			{ PaintTool::Erase,     "Erase", "Clear cells (E)",                    ImGuiKey_E },
			{ PaintTool::Fill,      "Fill",  "Flood-fill matching cells (F)",      ImGuiKey_F },
			{ PaintTool::Rectangle, "Rect",  "Drag a filled rectangle (R)",        ImGuiKey_R },
		};

		// Tool shortcuts, only with this window focused and no text box active
		const ImGuiIO& io = ImGui::GetIO();
		const bool shortcutsOk = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
			&& !io.WantTextInput && !io.KeyCtrl;

		for (const ToolInfo& t : tools)
		{
			if (shortcutsOk && ImGui::IsKeyPressed(t.key, false))
				m_tool = t.tool;

			const bool active = (m_tool == t.tool);
			if (active)
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

			if (ImGui::Button(t.label))
				m_tool = t.tool;

			if (active)
				ImGui::PopStyleColor();
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", t.tip);

			ImGui::SameLine();
		}

		ImGui::SetNextItemWidth(100.0f);
		ImGui::SliderInt("Zoom", &m_zoom, 1, 6);

		if (const char* reason = PaintBlockedReason())
		{
			ImGui::SameLine();
			ImGui::TextDisabled("%s", reason);
		}
	}

	void RoomEditor::TilePalette()
	{
		if (!m_tileDatabase)
		{
			ImGui::TextDisabled("Tile database not loaded");
			return;
		}

		const auto& tiles = m_tileDatabase->All();

		// The brush can point at a tile that was removed in the Tiles Editor
		if (m_brushTile != 0 && !m_tileDatabase->Get(m_brushTile))
			m_brushTile = tiles.empty() ? TileID(0) : tiles.begin()->first;

		if (m_brushTile == 0)
			ImGui::TextUnformatted("Brush: empty (eraser)");
		else
			ImGui::Text("Brush: %u %s", static_cast<unsigned>(m_brushTile), m_tileDatabase->Get(m_brushTile)->name.c_str());

		ImGui::BeginChild("TilePalette", ImVec2(0, 0), ImGuiChildFlags_Borders);

		constexpr float kCell = 28.0f;
		constexpr float kGap = 2.0f;
		const float avail = ImGui::GetContentRegionAvail().x;
		const int columns = (std::max) (1, static_cast<int>((avail + kGap) / (kCell + kGap)));

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(kGap, kGap));
		ImDrawList* dl = ImGui::GetWindowDrawList();

		auto selectBrush = [&](TileID id) {
			m_brushTile = id;
			if (m_tool == PaintTool::Erase && id != 0)
				m_tool = PaintTool::Draw; // picking a tile means you want to paint it
		};

		// Cell 0 is always the eraser (tile ID 0)
		int index = 0;
		{
			ImGui::PushID(0);
			if (ImGui::InvisibleButton("##empty", ImVec2(kCell, kCell)))
				selectBrush(0);

			const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
			dl->AddRectFilled(a, b, IM_COL32(40, 40, 45, 255));
			dl->AddLine(a, b, IM_COL32(200, 80, 80, 255), 2.0f);
			dl->AddLine(ImVec2(a.x, b.y), ImVec2(b.x, a.y), IM_COL32(200, 80, 80, 255), 2.0f);
			if (m_brushTile == 0)
				dl->AddRect(a, b, IM_COL32(255, 220, 60, 255), 0.0f, 0, 2.0f);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Empty (eraser)");
			ImGui::PopID();
		}
		++index;

		const TilesetInfo& tileset = m_tileDatabase->Tileset();
		const TileSprites& sprites = m_tileDatabase->Sprites();

		for (const auto& [id, def] : tiles) // ascending by ID
		{
			if ((index % columns) != 0)
				ImGui::SameLine();

			ImGui::PushID(static_cast<int>(id));
			if (ImGui::InvisibleButton("##tile", ImVec2(kCell, kCell)))
				selectBrush(id);

			const ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
			DrawTileImage(dl, a, b, &def, sprites, tileset, id, 255);

			if (id == m_brushTile)
				dl->AddRect(a, b, IM_COL32(255, 220, 60, 255), 0.0f, 0, 2.0f);
			else if (ImGui::IsItemHovered())
				dl->AddRect(a, b, IM_COL32(255, 255, 255, 160));

			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%u: %s", static_cast<unsigned>(id), def.name.c_str());

			ImGui::PopID();
			++index;
		}

		ImGui::PopStyleVar();
		ImGui::EndChild(); // always called
	}

	const char* RoomEditor::PaintBlockedReason() const
	{
		switch (m_activeLayer.kind)
		{
			case LayerKind::Visual:
			{
				const int i = m_activeLayer.index;
				if (i >= 0 && i < static_cast<int>(m_visualLayerVisible.size()) && !m_visualLayerVisible[i])
					return "Active layer is hidden";
				break;
			}
			case LayerKind::Gameplay:
				if (!m_showGameplay)
					return "Active layer is hidden";
				break;
			case LayerKind::Spawn:
				return "Spawn painting isn't available yet"; // needs its own palette (SpawnID)
		}
		return nullptr;
	}

	void RoomEditor::CancelStroke()
	{
		m_stroking = false;
		m_rectDragging = false;
	}

	// The only function that writes to a grid while editing.
	void RoomEditor::SetCell(Grid<TileID>& g, int x, int y, TileID id)
	{
		if (!g.InBounds(x, y) || g.At(x, y) == id)
			return;
		g.At(x, y) = id;
		m_dirty = true;
	}

	// Bresenham, so a fast mouse drag doesn't leave gaps between frames
	void RoomEditor::PaintLine(Grid<TileID>& g, int x0, int y0, int x1, int y1, TileID id)
	{
		const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
		const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
		int err = dx + dy;

		for (;;)
		{
			SetCell(g, x0, y0, id);
			if (x0 == x1 && y0 == y1)
				break;
			const int e2 = 2 * err;
			if (e2 >= dy) { err += dy; x0 += sx; }
			if (e2 <= dx) { err += dx; y0 += sy; }
		}
	}

	void RoomEditor::FloodFill(Grid<TileID>& g, int sx, int sy, TileID id)
	{
		if (!g.InBounds(sx, sy))
			return;

		const TileID target = g.At(sx, sy);
		if (target == id)
			return;

		std::vector<std::pair<int, int>> stack;
		SetCell(g, sx, sy, id); // cells are recolored when pushed, so none is pushed twice
		stack.emplace_back(sx, sy);

		while (!stack.empty())
		{
			const auto [x, y] = stack.back();
			stack.pop_back();

			const int nx[4] = { x + 1, x - 1, x, x };
			const int ny[4] = { y, y, y + 1, y - 1 };
			for (int i = 0; i < 4; ++i)
			{
				if (g.InBounds(nx[i], ny[i]) && g.At(nx[i], ny[i]) == target)
				{
					SetCell(g, nx[i], ny[i], id);
					stack.emplace_back(nx[i], ny[i]);
				}
			}
		}
	}

	void RoomEditor::FillRect(Grid<TileID>& g, int x0, int y0, int x1, int y1, TileID id)
	{
		for (int y = (std::min) (y0, y1); y <= (std::max) (y0, y1); ++y)
			for (int x = (std::min) (x0, x1); x <= (std::max) (x0, x1); ++x)
				SetCell(g, x, y, id);
	}

	// Must be called immediately after the canvas InvisibleButton.
	void RoomEditor::CanvasInput(const ImVec2& origin, float cell)
	{
		Grid<TileID>* grid = ActiveTileGrid();
		if (!grid || PaintBlockedReason())
		{
			CancelStroke();
			return;
		}

		const ImVec2 mouse = ImGui::GetMousePos();
		const int cx = static_cast<int>(std::floor((mouse.x - origin.x) / cell));
		const int cy = static_cast<int>(std::floor((mouse.y - origin.y) / cell));
		const int clampedX = std::clamp(cx, 0, grid->width - 1);
		const int clampedY = std::clamp(cy, 0, grid->height - 1);

		const TileID id = (m_tool == PaintTool::Erase) ? TileID(0) : m_brushTile;

		if (ImGui::IsItemActivated()) // mouse went down on the canvas this frame
		{
			switch (m_tool)
			{
				case PaintTool::Draw:
				case PaintTool::Erase:
					m_stroking = true;
					m_lastX = cx;
					m_lastY = cy;
					SetCell(*grid, cx, cy, id);
					break;
				case PaintTool::Fill:
					FloodFill(*grid, cx, cy, id);
					break;
				case PaintTool::Rectangle:
					m_rectDragging = true;
					m_rectX0 = clampedX;
					m_rectY0 = clampedY;
					break;
			}
		}
		else if (ImGui::IsItemActive() && m_stroking)
		{
			PaintLine(*grid, m_lastX, m_lastY, cx, cy, id);
			m_lastX = cx;
			m_lastY = cy;
		}

		if (m_rectDragging && ImGui::IsKeyPressed(ImGuiKey_Escape, false))
			m_rectDragging = false; // cancel without painting

		if (ImGui::IsItemDeactivated()) // mouse released
		{
			if (m_rectDragging)
				FillRect(*grid, m_rectX0, m_rectY0, clampedX, clampedY, id);
			CancelStroke();
		}
	}
}