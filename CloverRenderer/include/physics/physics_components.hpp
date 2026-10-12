#pragma once

#include "Box2D/types.h"
#include "core/serialization.hpp"

inline void to_json(nlohmann::json& j, const b2Vec2& v) { j = nlohmann::json::array({ v.x, v.y }); }
inline void from_json(const nlohmann::json& j, b2Vec2& v) { j.at(0).get_to(v.x); j.at(1).get_to(v.y); }

namespace clvr
{
	struct RigidBody
	{
		b2BodyId bodyId = b2_nullBodyId; // shouldn't be serialized, as it is managed by the physics world
		b2BodyType bodyType = b2_staticBody;
		float linearDamping = 0.0f;
		float angularDamping = 0.0f;
		float gravityScale = 1.0f;
		bool fixedRotation = false;
		bool isBullet = false;
		bool enableSleep = true;

		bool dirty = true;
		void Inspect();
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RigidBody, bodyType, linearDamping, angularDamping, gravityScale, fixedRotation, isBullet, enableSleep)

	struct BoxCollider
	{
		b2Vec2 halfExtents = { 0.5f, 0.5f };
		b2Vec2 offset = { 0.0f, 0.0f };
		bool isSensor = false;

		b2ShapeId shapeId = b2_nullShapeId;
		bool dirty = true;

		void Inspect();
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BoxCollider, halfExtents, offset, isSensor)

	struct CircleCollider
	{
		float radius = 0.5f;
		b2Vec2 offset = { 0.0f, 0.0f };
		bool isSensor = false;

		b2ShapeId shapeId = b2_nullShapeId;
		bool dirty = true;

		void Inspect();
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CircleCollider, radius, offset, isSensor)

	struct PolygonCollider
	{
		std::vector<b2Vec2> vertices;  // convex, max 8
		bool isSensor = false;

		b2ShapeId shapeId = b2_nullShapeId;
		bool dirty = true;

		void Inspect();
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PolygonCollider, vertices, isSensor)

	struct Material;
}

SAVEABLE_COMPONENT(clvr::RigidBody, "RigidBody");
SAVEABLE_COMPONENT(clvr::BoxCollider, "BoxCollider");
SAVEABLE_COMPONENT(clvr::CircleCollider, "CircleCollider");
SAVEABLE_COMPONENT(clvr::PolygonCollider, "PolygonCollider");