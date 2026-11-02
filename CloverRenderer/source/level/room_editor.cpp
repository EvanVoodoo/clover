#include "level/room_editor.hpp"
#include "core/imgui_helpers.hpp"
#include "level/room_serialization.hpp"

#include <core/engine.hpp>

namespace clvr
{
	RoomEditor::RoomEditor() : EditorWindow("Room Editor", ImGuiWindowFlags_MenuBar)
	{
		title = Title();
		priority = 25;

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
	}

	void RoomEditor::OnOpen()
	{}

	void RoomEditor::OnClose()
	{}

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