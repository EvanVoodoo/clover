#pragma once

#include "imgui/imgui.h"

namespace clvr {

	namespace ImGuiHelpers
	{
        constexpr ImVec2 POPUP_WINDOW_SIZE = ImVec2(960, 540);
	}

	static void Tooltip(const char* text)
	{
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", text);
	}

    template <typename DrawContent>
    void ModalPopup(const char* name, bool& openRequest, ImVec2 size, DrawContent&& drawContent,
                    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize)
    {
        if (openRequest)
        {
            ImGui::OpenPopup(name);
            openRequest = false;
        }

        // These apply to the next Begin. If the popup isn't open, ImGui discards them.
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);

        if (ImGui::BeginPopupModal(name, nullptr, flags))
        {
            drawContent();
            ImGui::EndPopup();
        }
    }
};