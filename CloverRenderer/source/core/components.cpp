#include "core/components.hpp"
#include <imgui.h>
#include <core/transform.hpp>
#include <core/engine.hpp>

using namespace clvr;

void PlayerComponent::Inspect() 
{
	if (ImGui::TreeNode("Player Component"))
	{
		ImGui::DragFloat("Speed", &speed, 1.0f, 0.0f, 10000.0f);
		ImGui::TreePop();
	}
}

void CameraComponent::Inspect()
{
	if (ImGui::TreeNode("Camera Component"))
	{
		ImGui::Checkbox("Follow Target", &followTarget);
		ImGui::DragFloat("Follow Speed", &followSpeed, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat2("Target Position", &targetPosition.x, 1.0f);
		//if (followTarget && followEntity != entt::null) 
		//{
		//	// get transform of the followed entity to display its name
		//	Transform& followedTransform = Engine.GetECS()->GetRegistry().get<Transform>(followEntity);
		//	ImGui::Text("Following Entity: %s", followedTransform.name);
		//}
		//else
		//	ImGui::Text("Not following any entity");
		ImGui::TreePop();
	}
}
