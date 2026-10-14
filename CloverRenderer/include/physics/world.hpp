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
		void CreateShape(PhysicsMaterial mat, bool isSensor, b2ShapeDef& out);
        void CreateShapes(Entity e, b2BodyId body);
		void UpdateShapeMaterial(b2ShapeId shape, const PhysicsMaterial& mat);

		void UpdatePlayerMovement(float dt);

		// Physics body manipulation

		/// <summary>
		/// Returns the current velocity of the body.
		/// </summary>
		/// <param name="rb">Rigidbody</param>
		/// <returns></returns>
		b2Vec2 GetVelocity(RigidBody rb) const;

		/// <summary>
		/// Returns the new velocity of the body after adding the specified value to it.
		/// </summary>
		/// <param name="rb">Rigidbody</param>
		/// <param name="velocity">Velocity to be added</param>
		/// <returns></returns>
		b2Vec2 AddVelocity(RigidBody rb, const b2Vec2 velocity);

		b2Vec2 AddVelocityX(RigidBody rb, float velocityX);

		b2Vec2 AddVelocityY(RigidBody rb, float velocityY);

		/// <summary>
		/// Returns the new velocity of the body after setting it to the specified value.
		/// </summary>
		/// <param name="rb">Rigidbody</param>
		/// <param name="velocity">New velocity</param>
		/// <returns></returns>
		b2Vec2 SetVelocity(RigidBody rb, const b2Vec2 velocity);

		b2Vec2 SetVelocityX(RigidBody rb, float velocityX);

		b2Vec2 SetVelocityY(RigidBody rb, float velocityY);

		/// <summary>
		/// Applies a force to the body at a specific point. This will also wake up the body if it is sleeping.
		/// </summary>
		/// <param name="rb"></param>
		/// <param name="force"></param>
		/// <param name="point"></param>
		/// <param name="wake"></param>
		void ApplyForce(RigidBody rb, const b2Vec2 force, const b2Vec2 point, bool wake);

		/// <summary>
		/// Applies an impulse to the body at a specific point. This will also wake up the body if it is sleeping.
		/// </summary>
		/// <param name="rb"></param>
		/// <param name="impulse"></param>
		/// <param name="point"></param>
		/// <param name="wake"></param>
		void ApplyImpulse(RigidBody rb, const b2Vec2 impulse, const b2Vec2 point, bool wake);

	private:
		b2WorldId m_worldId = b2_nullWorldId;
		b2ContactEvents m_contactEvents;
		b2SensorEvents m_sensorEvents;

		b2Vec2 m_gravity = { 0.0f, -9.81f };
		int m_substeps = 4;
		float m_accumulator = 0.0f;
	};
}