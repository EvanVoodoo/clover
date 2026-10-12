#pragma once

#include "core/ecs.hpp"
#include "Box2D/box2d.h"
#include "physics_components.hpp"
#include <core/transform.hpp>

namespace clvr
{
	class World : public System {
	public:
		World();
		~World();

		// System overrides
		void Update(float) override;
		void Render() override;
		void Inspect(float) override;

		RunMode GetRunMode() const { return RunMode::Always; }

		void OnPlayStart() override;

		json Save() override;
		void Load(const json&) override;

		// Physics world management

		b2WorldId GetWorldId() const { return m_worldId; }

		void SyncColliders();
		void SyncBodies();
		b2BodyId CreatePhysicsBody(const RigidBody& rb, const Transform& t);
        void CreateShapes(Entity e, b2BodyId body);

	private:
		b2WorldId m_worldId = b2_nullWorldId;
		b2ContactEvents m_contactEvents;
		b2SensorEvents m_sensorEvents;

		b2Vec2 m_gravity = { 0.0f, -9.81f };
		int m_substeps = 4;
	};
}