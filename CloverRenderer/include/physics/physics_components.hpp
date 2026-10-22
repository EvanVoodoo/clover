#pragma once

#include "Box2D/types.h"
#include "core/serialization.hpp"
#include <memory>

inline void to_json(nlohmann::json& j, const b2Vec2& v) { j = nlohmann::json::array({ v.x, v.y }); }
inline void from_json(const nlohmann::json& j, b2Vec2& v) { j.at(0).get_to(v.x); j.at(1).get_to(v.y); }

namespace clvr
{
	struct PhysicsMaterial
	{
		float density = 1.0f;
		float friction = 0.6f;
		float restitution = 0.0f;

		// optional, depending on your Box2D version
		float rollingResistance = 0.0f;
		float tangentSpeed = 0.0f;

		// event flags (shape-level in Box2D)
		bool enableContactEvents = true;
		bool enableSensorEvents = true;
		bool enableHitEvents = false;

		bool dirty = true;

		void Inspect();
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PhysicsMaterial, density, friction, restitution, rollingResistance, tangentSpeed, enableContactEvents, enableSensorEvents, enableHitEvents)

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
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RigidBody, bodyType, linearDamping, angularDamping, gravityScale, fixedRotation, isBullet, enableSleep)

	struct ShapeUserData
	{
		Entity entity;
	};

	struct BoxCollider
	{
		b2Vec2 halfExtents = { 0.5f, 0.5f };
		b2Vec2 offset = { 0.0f, 0.0f };
		bool isSensor = false;

		PhysicsMaterial material;

		b2ShapeId shapeId = b2_nullShapeId;
		std::unique_ptr<ShapeUserData> userData;
		bool dirty = true;

		void Inspect();
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BoxCollider, halfExtents, offset, isSensor, material)

	struct CircleCollider
	{
		float radius = 0.5f;
		b2Vec2 offset = { 0.0f, 0.0f };
		bool isSensor = false;

		PhysicsMaterial material;
		
		b2ShapeId shapeId = b2_nullShapeId;
		std::unique_ptr<ShapeUserData> userData;
		bool dirty = true;

		void Inspect();
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CircleCollider, radius, offset, isSensor, material)

	struct PolygonCollider
	{
		std::vector<b2Vec2> vertices;  // convex, max 8
		bool isSensor = false;

		PhysicsMaterial material;

		b2ShapeId shapeId = b2_nullShapeId;
		std::unique_ptr<ShapeUserData> userData;
		bool dirty = true;

		void Inspect();
	};
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PolygonCollider, vertices, isSensor, material)
}

SAVEABLE_COMPONENT(clvr::RigidBody, "RigidBody");
SAVEABLE_COMPONENT(clvr::BoxCollider, "BoxCollider");
SAVEABLE_COMPONENT(clvr::CircleCollider, "CircleCollider");
SAVEABLE_COMPONENT(clvr::PolygonCollider, "PolygonCollider");