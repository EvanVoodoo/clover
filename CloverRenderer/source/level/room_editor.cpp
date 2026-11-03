#include "level/room_editor.hpp"
#include "core/imgui_helpers.hpp"
#include "level/room_serialization.hpp"

#include <core/engine.hpp>
#include <cmath>

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
	}

	RoomEditor::~RoomEditor()
	{}

	void RoomEditor::Update(float dt)
	{}

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
		if (!IsValidRoomName(name)) { m_status = "Use letters, digits, _ and - only"; return false; }

		const auto path = RoomPath(name);
		if (std::filesystem::exists(path)) { m_status = "That room already exists"; return false; }

		RoomData room;
		if (!SaveRoomToFile(path, room)) { m_status = "Could not write " + path.string(); return false; }

		m_currentRoom = std::move(room);
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

		ImGui::SetNextItemWidth(120.0f);
		ImGui::SliderInt("Zoom", &m_zoom, 1, 6);

		// Left panel: layers (the tile palette will go below it later)
		ImGui::BeginChild("LeftPanel", ImVec2(180.0f, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
		LayerPanel();
		ImGui::EndChild();

		ImGui::SameLine();

		
		DrawCanvas();
	}

	void RoomEditor::OnOpen()
	{}

	void RoomEditor::OnClose()
	{}

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
		const float cell = 16.0f * m_zoom; // 16 should be a constant somewhere
		const int w = m_currentRoom.width;
		const int h = m_currentRoom.height;
		const ImVec2 canvasSize(w * cell, h * cell);

		ImGui::BeginChild("Canvas", ImVec2(0, 0), ImGuiChildFlags_Borders,
						  ImGuiWindowFlags_HorizontalScrollbar);

		const ImVec2 origin = ImGui::GetCursorScreenPos(); // already accounts for scroll
		ImGui::InvisibleButton("##canvas", canvasSize,
							   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		const bool hovered = ImGui::IsItemHovered();

		ImDrawList* dl = ImGui::GetWindowDrawList();
		auto cellMin = [&](int x, int y) { return ImVec2(origin.x + x * cell, origin.y + y * cell); };
		auto cellMax = [&](int x, int y) { return ImVec2(origin.x + (x + 1) * cell, origin.y + (y + 1) * cell); };

		// Room background
		dl->AddRectFilled(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y),
						  IM_COL32(25, 25, 30, 255));

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
						dl->AddRectFilled(cellMin(x, y), cellMax(x, y), IdColor(id, alpha));
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

		// Grid lines and room border
		const ImU32 gridColor = IM_COL32(255, 255, 255, 30);
		for (int x = 0; x <= w; ++x)
			dl->AddLine(ImVec2(origin.x + x * cell, origin.y),
						ImVec2(origin.x + x * cell, origin.y + canvasSize.y), gridColor);
		for (int y = 0; y <= h; ++y)
			dl->AddLine(ImVec2(origin.x, origin.y + y * cell),
						ImVec2(origin.x + canvasSize.x, origin.y + y * cell), gridColor);
		dl->AddRect(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y),
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
}