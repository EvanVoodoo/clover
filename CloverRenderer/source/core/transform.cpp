#include "core/transform.hpp"
#include "imgui.h"
#include <core/engine.hpp>
#include <physics/physics_components.hpp>
#include <entt/entity/helper.hpp>
#include <Box2D/Box2D.h>
#include "physics/units.hpp"

namespace clvr {
    XMMATRIX Transform::GetWorld() const
    {
        return GetWorld(Engine.GetECS()->GetRegistry());
    }

    XMMATRIX Transform::GetWorld(const entt::registry& registry) const
    {
        XMMATRIX world = GetLocal();

        if (parent != entt::null && registry.valid(parent)) {
            const Transform* t = registry.try_get<Transform>(parent);
            if (t) {
                world = world * t->GetWorld(registry);
            }
        }
        return world;
    }

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
                                        Units::ToMeters({ GetWorldPosition().x, GetWorldPosition().y}),
                                        b2MakeRot(GetWorldRotation()));
                    b2Body_SetAwake(rb->bodyId, true);
                }
            }

            ImGui::TreePop();
        }
        return;
    }

	bool IsAncestorOrSelf(entt::entity ancestor, entt::entity node)
	{
		auto& registry = Engine.GetECS()->GetRegistry();
		entt::entity current = node;
		while (current != entt::null && registry.valid(current))
		{
			if (current == ancestor)
				return true;
			const Transform* t = registry.try_get<Transform>(current);
			if (!t)
				break;
			current = t->parent;
		}
		return false;
	}

	bool SetParent(entt::entity child, entt::entity newParent, bool keepWorld)
	{
		auto& registry = Engine.GetECS()->GetRegistry();
		Transform* t = registry.try_get<Transform>(child);
		if (!t)
			return false;

		if (newParent != entt::null)
		{
			if (!registry.valid(newParent) || !registry.all_of<Transform>(newParent))
				return false;
			if (IsAncestorOrSelf(child, newParent))
				return false; // would create a cycle (includes parenting to itself)
		}

		const entt::entity oldParent = t->parent;
		const XMMATRIX oldWorld = t->GetWorld(registry);
		t->parent = newParent;

		if (!keepWorld)
			return true;

		XMMATRIX parentWorld = XMMatrixIdentity();
		if (newParent != entt::null)
			parentWorld = registry.get<Transform>(newParent).GetWorld(registry);

		// world = local * parentWorld  =>  local = world * inverse(parentWorld)
		const XMMATRIX newLocal = oldWorld * XMMatrixInverse(nullptr, parentWorld);

		XMVECTOR s, q, p;
		if (!XMMatrixDecompose(&s, &q, &p, newLocal))
		{
			t->parent = oldParent; // e.g. a parent with zero scale: leave everything as it was
			return false;
		}

		XMFLOAT3 sc, pos;
		XMFLOAT4 quat;
		XMStoreFloat3(&sc, s);
		XMStoreFloat3(&pos, p);
		XMStoreFloat4(&quat, q);

		t->position = XMFLOAT2(pos.x, pos.y);
		t->scale = XMFLOAT2(sc.x, sc.y);
		t->rotation = 2.0f * std::atan2(quat.z, quat.w); // quaternion about Z -> angle
		return true;
	}

	std::vector<entt::entity> GetChildren(entt::entity parent)
	{
		auto& registry = Engine.GetECS()->GetRegistry();
		std::vector<entt::entity> children;
		for (auto [entity, t] : registry.view<const Transform>().each())
			if (t.parent == parent)
				children.push_back(entity);
		return children;
	}

	// Destroys the entity and everything below it.
	void DestroyHierarchy(entt::entity root)
	{
		auto& registry = Engine.GetECS()->GetRegistry();
		std::vector<entt::entity> stack{ root }, doomed;
		while (!stack.empty())
		{
			const entt::entity e = stack.back();
			stack.pop_back();
			doomed.push_back(e);
			for (entt::entity c : GetChildren(e))
				stack.push_back(c);
		}
		registry.destroy(doomed.begin(), doomed.end());
	}
}