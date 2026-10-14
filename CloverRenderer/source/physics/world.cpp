#include "physics/world.hpp"
#include <imgui.h>
#include <core/engine.hpp>
#include <physics/physics_components.hpp>
#include "physics/units.hpp"
#include <core/transform.hpp>
#include <core/components.hpp>

namespace clvr {
	World::World()
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = m_gravity;
		m_worldId = b2CreateWorld(&worldDef);
		m_substeps = 4;

		title = "Physics World";
	}

	World::~World()
	{
		b2DestroyWorld(m_worldId);
	}

	void World::Update(float dt)
	{
		// fixed time step
		if (Engine.GetEngineMode() != EngineMode::Playing)
			return;

		SyncBodies();
		SyncColliders();

		constexpr float fixedStep = 1.0f / 60.0f;
		m_accumulator += std::fmin(dt, 0.25f);

		while (m_accumulator >= fixedStep)
		{
			UpdatePlayerMovement(fixedStep);            // same dt as the step it feeds
			b2World_Step(m_worldId, fixedStep, m_substeps);
			m_accumulator -= fixedStep;
		}

		for (auto [entity, t, rb] : Engine.GetECS()->GetRegistry().view<Transform, RigidBody>().each())
		{
			b2BodyId body = rb.bodyId;

			// check if static body, if so, skip updating transform
			if (rb.bodyType == b2BodyType::b2_staticBody)
				continue;

			if (b2Body_IsValid(body))
			{
				b2Vec2 pos = b2Body_GetPosition(body);
				t.position.x = Units::ToPixels(pos.x);
				t.position.y = Units::ToPixels(pos.y);
				t.rotation = b2Rot_GetAngle(b2Body_GetRotation(body));
			}
		}
	}

	void World::Render()
	{
		// No rendering for physics world
	}

	void World::Inspect(float dt)
	{
		ImGui::Begin("Physics World");
	
		ImGui::DragInt("Substeps", &m_substeps, 0.25f, 1, 16);

		if (ImGui::DragFloat2("Gravity", &m_gravity.x, 0.1f, -100.0f, 100.0f)) {
			// Update the gravity in the physics world
			b2World_SetGravity(m_worldId, m_gravity);
			// wake up all bodies in the world to apply new gravity
			for (auto [entity, rb] : Engine.GetECS()->GetRegistry().view<RigidBody>().each())
			{
				if (b2Body_IsValid(rb.bodyId))
					b2Body_SetAwake(rb.bodyId, true);
			}
		}

		ImGui::End();
	}

	void World::OnPlayStart() 
	{
		m_accumulator = 0.0f;
		// on play start, we need to create physics bodies for all entities that have a RigidBody component
		auto ecs = Engine.GetECS();
		for (auto [entity, rb, t] : ecs->GetRegistry().view<RigidBody, Transform>().each())
		{
			rb.bodyId = CreatePhysicsBody(rb, t);
			CreateShapes(entity, rb.bodyId);
		}
	}

	json World::Save() {
		// Save gravity and substeps
		return json{
			{ "gravity", { m_gravity.x, m_gravity.y } },
			{ "substeps", m_substeps }
		};
	}

	void World::Load(const json& j) {
		// Load gravity and substeps
		if (j.contains("gravity") && j["gravity"].is_array() && j["gravity"].size() == 2) {
			m_gravity.x = j["gravity"][0].get<float>();
			m_gravity.y = j["gravity"][1].get<float>();
			b2World_SetGravity(m_worldId, m_gravity);
		}
		if (j.contains("substeps") && j["substeps"].is_number()) {
			m_substeps = j["substeps"].get<int>();
		}
	}

	void World::SyncColliders()
	{
		auto& reg = Engine.GetECS()->GetRegistry();

		for (auto [entity, box, rb] : reg.view<BoxCollider, RigidBody>().each())
		{
			if (!box.dirty)
				continue;
			if (!b2Shape_IsValid(box.shapeId) || B2_IS_NULL(box.shapeId)) {
				// if playing, create the shape if it doesn't exist
				if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (b2Body_IsValid(rb.bodyId)) {
						b2ShapeDef def;
						CreateShape(box.material, box.isSensor, def);
						b2Polygon poly = b2MakeOffsetBox(
							Units::ToMeters(box.halfExtents.x),
							Units::ToMeters(box.halfExtents.y),
							Units::ToMeters(box.offset),
							b2Rot_identity);
						box.shapeId = b2CreatePolygonShape(rb.bodyId, &def, &poly);
						box.dirty = false;
					}
					else continue;
				}
				else continue;
			}

			b2Polygon poly = b2MakeOffsetBox(
				Units::ToMeters(box.halfExtents.x),
				Units::ToMeters(box.halfExtents.y),
				Units::ToMeters(box.offset),
				b2Rot_identity);
			b2Shape_SetPolygon(box.shapeId, &poly);
			UpdateShapeMaterial(box.shapeId, box.material);
			box.dirty = false;
		}

		for (auto [entity, circle, rb] : reg.view<CircleCollider, RigidBody>().each())
		{
			if (!circle.dirty)
				continue;
			if (!b2Shape_IsValid(circle.shapeId) || B2_IS_NULL(circle.shapeId)) {
				if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (b2Body_IsValid(rb.bodyId)) {
						b2ShapeDef def;
						CreateShape(circle.material, circle.isSensor, def);
						b2Circle c = { Units::ToMeters(circle.offset), Units::ToMeters(circle.radius) };
						circle.shapeId = b2CreateCircleShape(rb.bodyId, &def, &c);
						circle.dirty = false;
					}
					else continue;
				}
				else continue;
			}

			b2Circle c = { Units::ToMeters(circle.offset), Units::ToMeters(circle.radius) };
			b2Shape_SetCircle(circle.shapeId, &c);
			UpdateShapeMaterial(circle.shapeId, circle.material);
			circle.dirty = false;
		}

		for (auto [entity, poly, rb] : reg.view<PolygonCollider, RigidBody>().each())
		{
			if (!poly.dirty)
				continue;
			if (!b2Shape_IsValid(poly.shapeId) || B2_IS_NULL(poly.shapeId)) {
				if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (b2Body_IsValid(rb.bodyId)) {
						b2Hull hull = b2ComputeHull(poly.vertices.data(), (int)poly.vertices.size());
						if (hull.count > 0)
						{
							b2ShapeDef def;
							CreateShape(poly.material, poly.isSensor, def);
							b2Polygon p = b2MakePolygon(&hull, 0.0f);
							poly.shapeId = b2CreatePolygonShape(rb.bodyId, &def, &p);
							poly.dirty = false;
						}
					}
					else continue;
				}
				else continue;
			}
			
			b2Hull hull = b2ComputeHull(poly.vertices.data(), (int)poly.vertices.size());
			if (hull.count > 0)
			{
				b2Polygon p = b2MakePolygon(&hull, 0.0f);
				b2Shape_SetPolygon(poly.shapeId, &p);
				UpdateShapeMaterial(poly.shapeId, poly.material);
				poly.dirty = false;
			}
		}
	}

	void World::SyncBodies()
	{
		auto& reg = Engine.GetECS()->GetRegistry();
		for (auto [entity, rb, t] : reg.view<RigidBody, Transform>().each())
		{
			if (!rb.dirty)
				continue;

			if (!b2Body_IsValid(rb.bodyId)) {
				if (Engine.GetEngineMode() == EngineMode::Playing) {
					rb.bodyId = CreatePhysicsBody(rb, t);
					CreateShapes(entity, rb.bodyId);
				}
				else continue;
			}

			b2Body_SetType(rb.bodyId, rb.bodyType);
			b2Body_SetLinearDamping(rb.bodyId, rb.linearDamping);
			b2Body_SetAngularDamping(rb.bodyId, rb.angularDamping);
			b2Body_SetGravityScale(rb.bodyId, rb.gravityScale);
			b2Body_SetFixedRotation(rb.bodyId, rb.fixedRotation);
			b2Body_SetBullet(rb.bodyId, rb.isBullet);
			b2Body_EnableSleep(rb.bodyId, rb.enableSleep);
		}
	}

	b2BodyId World::CreatePhysicsBody(const RigidBody& rb, const Transform& t)
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = rb.bodyType;
		bodyDef.position = Units::ToMeters({ t.position.x, t.position.y });
		bodyDef.rotation = b2MakeRot(t.rotation);
		bodyDef.linearDamping = rb.linearDamping;
		bodyDef.angularDamping = rb.angularDamping;
		bodyDef.gravityScale = rb.gravityScale;
		bodyDef.fixedRotation = rb.fixedRotation;
		bodyDef.isBullet = rb.isBullet;
		bodyDef.enableSleep = rb.enableSleep;
		return b2CreateBody(m_worldId, &bodyDef);
	}

	void World::CreateShape(PhysicsMaterial mat, bool isSensor, b2ShapeDef& out) {
		out = b2DefaultShapeDef();
		out.density = mat.density;
		out.material.friction = mat.friction;
		out.material.restitution = mat.restitution;
		out.material.rollingResistance = mat.rollingResistance;
		out.material.tangentSpeed = mat.tangentSpeed;
		out.enableContactEvents = mat.enableContactEvents;
		out.enableSensorEvents = mat.enableSensorEvents;
		out.enableHitEvents = mat.enableHitEvents;
		out.isSensor = isSensor;
	}

	void World::CreateShapes(Entity e, b2BodyId body)
	{
		if (!b2Body_IsValid(body))
			return;

		auto& reg = Engine.GetECS()->GetRegistry();
		if (auto* box = reg.try_get<BoxCollider>(e))
		{
			b2ShapeDef def;
			CreateShape(box->material, box->isSensor, def);

			b2Polygon poly = b2MakeOffsetBox(Units::ToMeters(box->halfExtents.x),
				Units::ToMeters(box->halfExtents.y),
				{ Units::ToMeters(box->offset.x), Units::ToMeters(box->offset.y) },
				b2Rot_identity);
			box->shapeId = b2CreatePolygonShape(body, &def, &poly);
		}

		if (auto* circle = reg.try_get<CircleCollider>(e))
		{
			b2ShapeDef def = b2DefaultShapeDef();
			def.isSensor = circle->isSensor;
			b2Circle c = { Units::ToMeters(circle->offset), Units::ToMeters(circle->radius) };
			circle->shapeId = b2CreateCircleShape(body, &def, &c);
		}

		if (auto* poly = reg.try_get<PolygonCollider>(e))
		{
			b2Hull hull = b2ComputeHull(poly->vertices.data(), (int)poly->vertices.size());
			if (hull.count > 0)
			{
				b2ShapeDef def = b2DefaultShapeDef();
				def.isSensor = poly->isSensor;
				b2Polygon p = b2MakePolygon(&hull, 0.0f);
				poly->shapeId = b2CreatePolygonShape(body, &def, &p);
			}
		}
	}

	void World::UpdateShapeMaterial(b2ShapeId shape, const PhysicsMaterial& mat)
	{
		b2SurfaceMaterial material = b2Shape_GetSurfaceMaterial(shape);
		material.friction = mat.friction;
		material.restitution = mat.restitution;
		material.rollingResistance = mat.rollingResistance;
		material.tangentSpeed = mat.tangentSpeed;
		b2Shape_SetSurfaceMaterial(shape, material);
		b2Shape_SetDensity(shape, mat.density, true);
		b2Shape_EnableContactEvents(shape, mat.enableContactEvents);
		b2Shape_EnableSensorEvents(shape, mat.enableSensorEvents);
		b2Shape_EnableHitEvents(shape, mat.enableHitEvents);
	}

	void World::UpdatePlayerMovement(float dt)   // dt = fixed step
	{
		auto& reg = Engine.GetECS()->GetRegistry();
		for (auto [e, rb, p] : reg.view<RigidBody, PlayerComponent>().each())
		{
			if (!b2Body_IsValid(rb.bodyId))
				continue;

			b2Vec2 vel = GetVelocity(rb);                           // px/s
			b2Vec2 target = b2MulSV(p.maxSpeed * p.moveScale, p.moveInput);

			bool hasInput = b2LengthSquared(p.moveInput) > 0.0f;
			float rate = (hasInput ? p.acceleration : p.deceleration) * dt;

			b2Vec2 delta = b2Sub(target, vel);
			float dist = b2Length(delta);

			if (dist <= rate)
				vel = target;                                       // arrived
			else
				vel = b2Add(vel, b2MulSV(rate / dist, delta));      // step toward target

			SetVelocity(rb, vel);
		}
	}

	b2Vec2 World::GetVelocity(RigidBody rb) const
	{
		if (!b2Body_IsValid(rb.bodyId) || !b2Body_IsAwake(rb.bodyId))
			return { 0.0f, 0.0f };
		return Units::ToPixels(b2Body_GetLinearVelocity(rb.bodyId));
	}

	b2Vec2 World::AddVelocity(RigidBody rb, const b2Vec2 velocity)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, b2Body_GetLinearVelocity(rb.bodyId) + Units::ToMeters(velocity));
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	b2Vec2 World::AddVelocityX(RigidBody rb, float velocityX)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, b2Vec2(b2Body_GetLinearVelocity(rb.bodyId).x + Units::ToMeters(velocityX), b2Body_GetLinearVelocity(rb.bodyId).y));
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	b2Vec2 World::AddVelocityY(RigidBody rb, float velocityY)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, b2Vec2(b2Body_GetLinearVelocity(rb.bodyId).x, b2Body_GetLinearVelocity(rb.bodyId).y + Units::ToMeters(velocityY)));
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	b2Vec2 World::SetVelocity(RigidBody rb, const b2Vec2 velocity)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, Units::ToMeters(velocity));
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	b2Vec2 World::SetVelocityX(RigidBody rb, float velocityX)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, b2Vec2(Units::ToMeters(velocityX), b2Body_GetLinearVelocity(rb.bodyId).y));
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	b2Vec2 World::SetVelocityY(RigidBody rb, float velocityY)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, b2Vec2(b2Body_GetLinearVelocity(rb.bodyId).x, Units::ToMeters(velocityY)));
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	void World::ApplyForce(RigidBody rb, const b2Vec2 force, const b2Vec2 point, bool wake)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return;
		b2Body_ApplyForce(rb.bodyId, Units::ToMeters(force), Units::ToMeters(point), wake);
	}

	void World::ApplyImpulse(RigidBody rb, const b2Vec2 impulse, const b2Vec2 point, bool wake)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return;
		b2Body_ApplyLinearImpulse(rb.bodyId, Units::ToMeters(impulse), Units::ToMeters(point), wake);
	}
};