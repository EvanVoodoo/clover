#pragma once

#include <string>
#include <imgui.h>

namespace clvr
{
	class EditorWindow
	{
	public:
		explicit EditorWindow(std::string title, ImGuiWindowFlags flags = 0)
			: m_windowTitle(std::move(title)), m_flags(flags) {}

		virtual ~EditorWindow() = default;

		// Not virtual: the base owns the window lifecycle.
		void DrawWindow()
		{
			if (!m_open)
				return;

			const bool wasOpen = m_open;

			if (ImGui::Begin(m_windowTitle.c_str(), &m_open, m_flags))
				DrawContents();
			ImGui::End();

			if (wasOpen && !m_open)
				OnClose();
		}

		void Open()
		{
			if (m_open)
				return;
			m_open = true;
			OnOpen();
		}

		void Close()
		{
			if (!m_open)
				return;
			m_open = false;
			OnClose();
		}

		void Toggle() { m_open ? Close() : Open(); }

		bool IsOpen() const { return m_open; }
		bool& OpenFlag() { return m_open; }   // for MenuItem checkmarks
		const std::string& Title() const { return m_windowTitle; }

	protected:
		virtual void DrawContents() = 0;      // required
		virtual void OnOpen() {}              // optional hooks
		virtual void OnClose() {}

	private:
		std::string m_windowTitle;
		ImGuiWindowFlags m_flags;
		bool m_open = false;
	};

	class EditorWindowManager
	{
	public:
		void DrawMenu();

	private:
	};
}