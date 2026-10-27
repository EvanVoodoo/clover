#include "core/transform.hpp"
#include "imgui.h"
#include <core/engine.hpp>
#include <physics/physics_components.hpp>
#include <entt/entity/helper.hpp>
#include <Box2D/Box2D.h>
#include "physics/units.hpp"

using namespace clvr;

void Transform::Inspect()
{
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
    if (ImGui::TreeNodeEx("Transform", flags)) 
    {
        char nameBuffer[128];
		bool dirty = false;
        strncpy_s(nameBuffer, name.c_str(), sizeof(nameBuffer) - 1);
        if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer)))
        {
            name = nameBuffer;
			dirty = true;
        }

        ImGui::DragFloat2("Position", &position.x, 0.5f);
        ImGui::DragFloat2("Scale", &scale.x, 0.01f, 0.01f, 100.0f);

        float rotationDegrees = XMConvertToDegrees(rotation);
        if (ImGui::DragFloat("Rotation", &rotationDegrees, 1.0f)) {
            rotation = XMConvertToRadians(rotationDegrees);
			dirty = true;
        }

		auto& reg = Engine.GetECS()->GetRegistry();
        Entity e = entt::to_entity(reg.storage<Transform>(), *this);

		RigidBody* rb = reg.try_get<RigidBody>(e);
        if (rb) {
            if (b2Body_IsValid(rb->bodyId)) {
                b2Body_SetTransform(rb->bodyId,
                    Units::ToMeters({ position.x, position.y }),
                    b2MakeRot(rotation));
				b2Body_SetAwake(rb->bodyId, true);
            }
        }

		ImGui::TreePop();
    }
    return;
}
