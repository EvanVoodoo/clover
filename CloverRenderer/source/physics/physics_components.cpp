#include "physics/physics_components.hpp"
#include <imgui.h>
#include "core/imgui_helpers.hpp"
#include <Box2D/box2d.h>
#include <core/engine.hpp>
#include <physics/world.hpp>

using namespace clvr;

RigidBody::~RigidBody() {
	if (b2Body_IsValid(bodyId)) {
		Engine.GetECS()->GetSystem<World>().DestroyBody(bodyId);
	}
}

BoxCollider::~BoxCollider() {
	if (b2Shape_IsValid(shapeId)) {
		Engine.GetECS()->GetSystem<World>().DestroyShape(shapeId);
	}
}

CircleCollider::~CircleCollider() {
	if (b2Shape_IsValid(shapeId)) {
		Engine.GetECS()->GetSystem<World>().DestroyShape(shapeId);
	}
}

PolygonCollider::~PolygonCollider() {
	if (b2Shape_IsValid(shapeId)) {
		Engine.GetECS()->GetSystem<World>().DestroyShape(shapeId);
	}
}

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
		ImGui::DragFloat("Gravity Scale", &gravityScale, 0.1f, -10.0f, 10.0f);
		ImGui::Checkbox("Fixed Rotation", &fixedRotation);
		ImGui::Checkbox("Is Bullet", &isBullet);
		ImGui::Checkbox("Enable Sleep", &enableSleep);
		ImGui::TreePop();
	}
}

void BoxCollider::Inspect() {
	if (ImGui::TreeNode("Box Collider")) {
		material.Inspect();
		CollisionFilters::InspectCollisionCategory(category);
		if (material.dirty) {
			dirty = true;
			material.dirty = false;
		}
		if (ImGui::DragFloat2("Half Extents", &halfExtents.x, 0.1f, 0.0f, 10000.0f)) {
			dirty = true;
		}
		if (ImGui::DragFloat2("Offset", &offset.x, 0.1f, -10000.0f, 10000.0f)) {
			dirty = true;
		}
		if (ImGui::Checkbox("Is Sensor", &isSensor)) {
			dirty = true;
		}
		ImGui::TreePop();
	}
	Engine.GetECS()->GetSystem<World>().DrawShape(shapeId, isSensor ? b2_colorYellow : b2_colorGreen);
}

void CircleCollider::Inspect() {
	if (ImGui::TreeNode("Circle Collider")) {
		material.Inspect();
		CollisionFilters::InspectCollisionCategory(category);
		if (material.dirty) {
			dirty = true;
			material.dirty = false;
		}
		if (ImGui::DragFloat("Radius", &radius, 0.1f, 0.0f, 10000.0f)) {
			dirty = true;
		}
		if (ImGui::DragFloat2("Offset", &offset.x, 0.1f, -10000.0f, 10000.0f)) {
			dirty = true;
		}
		if (ImGui::Checkbox("Is Sensor", &isSensor)) {
			dirty = true;
		}
		ImGui::TreePop();
	}
	Engine.GetECS()->GetSystem<World>().DrawShape(shapeId, isSensor ? b2_colorYellow : b2_colorGreen);
}

void PolygonCollider::Inspect() {
	if (ImGui::TreeNode("Polygon Collider")) {
		material.Inspect();
		CollisionFilters::InspectCollisionCategory(category);
		if (material.dirty) {
			dirty = true;
			material.dirty = false;
		}
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
	Engine.GetECS()->GetSystem<World>().DrawShape(shapeId, isSensor ? b2_colorYellow : b2_colorGreen);
}

void PhysicsMaterial::Inspect()
{
	if (ImGui::TreeNode("Physics Material"))
	{
		if (ImGui::DragFloat("Density", &density, 0.01f, 0.0f, 100.0f))
			dirty = true;
		Tooltip("Mass = area x density. Only matters for dynamic bodies, and only relative to other bodies.");

		if (ImGui::DragFloat("Friction", &friction, 0.01f, 0.0f, 10.0f))
			dirty = true;
		Tooltip("Resistance to sliding. Use 0 for characters and projectiles, and rely on linear damping instead.");

		if (ImGui::SliderFloat("Restitution", &restitution, 0.0f, 1.0f))
			dirty = true;
		Tooltip("Bounciness. 0 = no bounce, 1 = perfectly elastic.");

		if (ImGui::DragFloat("Rolling Resistance", &rollingResistance, 0.01f, 0.0f, 1.0f))
			dirty = true;
		Tooltip("Slows rotation of circles in contact. Irrelevant for bodies with fixed rotation.");

		if (ImGui::DragFloat("Tangent Speed", &tangentSpeed, 0.1f, -100.0f, 100.0f))
			dirty = true;
		Tooltip("Conveyor-belt effect: pushes touching shapes sideways at this speed (m/s).");

		ImGui::SeparatorText("Events");

		if (ImGui::Checkbox("Contact Events", &enableContactEvents))
			dirty = true;
		if (ImGui::Checkbox("Sensor Events", &enableSensorEvents))
			dirty = true;
		Tooltip("Must be on for both the sensor and the shapes that should trigger it.");

		if (ImGui::Checkbox("Hit Events", &enableHitEvents))
			dirty = true;
		Tooltip("Reports collisions above a speed threshold. Useful for impact sounds and damage.");

		ImGui::TreePop();
	}
}
