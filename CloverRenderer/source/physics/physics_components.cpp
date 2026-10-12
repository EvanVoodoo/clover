#include "physics/physics_components.hpp"
#include <imgui.h>
#include <Box2D/box2d.h>

using namespace clvr;

void RigidBody::Inspect() {
	if (ImGui::TreeNode("RigidBody")) {
		const char* bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
		int currentType = static_cast<int>(bodyType);
		if (ImGui::Combo("Body Type", &currentType, bodyTypes, IM_ARRAYSIZE(bodyTypes))) {
			bodyType = static_cast<b2BodyType>(currentType);
			// Update the body type in the physics world if necessary
			if (b2Body_IsValid(bodyId)) {
				b2Body_SetType(bodyId, bodyType);
			}
		}
		//ImGui::Text("Body Type: %s", bodyType == b2_staticBody ? "Static" : bodyType == b2_kinematicBody ? "Kinematic" : "Dynamic");
		ImGui::DragFloat("Linear Damping", &linearDamping, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Angular Damping", &angularDamping, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("Gravity Scale", &gravityScale, 0.1f, 0.0f, 10.0f);
		ImGui::Checkbox("Fixed Rotation", &fixedRotation);
		ImGui::Checkbox("Is Bullet", &isBullet);
		ImGui::Checkbox("Enable Sleep", &enableSleep);
		ImGui::TreePop();
	}
}

void BoxCollider::Inspect() {
	if (ImGui::TreeNode("Box Collider")) {
		if (ImGui::DragFloat2("Half Extents", &halfExtents.x, 0.01f, 0.0f, 100.0f)) {
			dirty = true;
		}
		if (ImGui::DragFloat2("Offset", &offset.x, 0.01f, -100.0f, 100.0f)) {
			dirty = true;
		}
		if (ImGui::Checkbox("Is Sensor", &isSensor)) {
			dirty = true;
		}
		ImGui::TreePop();
	}
}

void CircleCollider::Inspect() {
	if (ImGui::TreeNode("Circle Collider")) {
		if (ImGui::DragFloat("Radius", &radius, 0.01f, 0.0f, 100.0f)) {
			dirty = true;
		}
		if (ImGui::DragFloat2("Offset", &offset.x, 0.01f, -100.0f, 100.0f)) {
			dirty = true;
		}
		if (ImGui::Checkbox("Is Sensor", &isSensor)) {
			dirty = true;
		}
		ImGui::TreePop();
	}
}

void PolygonCollider::Inspect() {
	if (ImGui::TreeNode("Polygon Collider")) {
		ImGui::Text("Vertices (Convex, Max 8):");
		for (size_t i = 0; i < vertices.size(); ++i) {
			ImGui::DragFloat2(("Vertex " + std::to_string(i)).c_str(), &vertices[i].x, 0.01f, -100.0f, 100.0f);
		}

		if (vertices.size() >= 8) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Add Vertex") && vertices.size() < 8) {
			vertices.push_back({ 0.0f, 0.0f });
			dirty = true;
		}
		if (vertices.size() >= 8) {
			ImGui::EndDisabled();
		}

		if (vertices.size() <= 0) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Remove Vertex") && !vertices.empty()) {
			vertices.pop_back();
			dirty = true;
		}
		if (vertices.size() <= 0) {
			ImGui::EndDisabled();
		}
		if (ImGui::Checkbox("Is Sensor", &isSensor)) {
			dirty = true;
		}
		ImGui::TreePop();
	}
}