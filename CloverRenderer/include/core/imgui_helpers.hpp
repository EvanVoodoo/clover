#pragma once

#include "imgui/imgui.h"

namespace clvr {
	static void Tooltip(const char* text)
	{
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", text);
	}
};