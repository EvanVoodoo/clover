#include "core/transform.hpp"
#include "imgui.h"

using namespace clvr;

void Transform::Inspect()
{
    if (ImGui::TreeNode("Transform")) 
    {
        char nameBuffer[128];
        strncpy_s(nameBuffer, name.c_str(), sizeof(nameBuffer) - 1);
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
            name = nameBuffer;

        ImGui::DragFloat2("Position", &position.x, 0.5f);
        ImGui::DragFloat2("Scale", &scale.x, 0.01f, 0.01f, 100.0f);

        float rotationDegrees = XMConvertToDegrees(rotation);
        if (ImGui::DragFloat("Rotation", &rotationDegrees, 1.0f))
            rotation = XMConvertToRadians(rotationDegrees);

		ImGui::TreePop();
    }
    return;
}
