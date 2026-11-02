#include "level/room_editor.hpp"

namespace clvr
{
	RoomEditor::RoomEditor() : EditorWindow("Room Editor", ImGuiWindowFlags_MenuBar)
	{
		title = Title();
		priority = 25;
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

	void RoomEditor::DrawContents()
	{
		//
	}

	void RoomEditor::OnOpen()
	{}

	void RoomEditor::OnClose()
	{}
}