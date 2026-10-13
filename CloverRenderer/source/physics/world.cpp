#include "physics/world.hpp"
#include <imgui.h>
#include <core/engine.hpp>
#include <physics/physics_components.hpp>
#include "physics/units.hpp"
#include <core/transform.hpp>

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

		SyncColliders();
		SyncBodies();

		b2World_Step(m_worldId, 1.0f / 60.0f, m_substeps);

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
				// for now, we won't handle rotation, but it can be added later if needed
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

		for (auto [entity, box] : reg.view<BoxCollider>().each())
		{
			if (!box.dirty)
				continue;
			box.dirty = false;
			if (!b2Shape_IsValid(box.shapeId)) {
				// if playing, create the shape if it doesn't exist
				if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (auto* rb = reg.try_get<RigidBody>(entity)) {
						if (b2Body_IsValid(rb->bodyId)) {
							b2ShapeDef def = b2DefaultShapeDef();
							def.isSensor = box.isSensor;
							b2Polygon poly = b2MakeOffsetBox(
								Units::ToMeters(box.halfExtents.x),
								Units::ToMeters(box.halfExtents.y),
								Units::ToMeters(box.offset),
								b2Rot_identity);
							box.shapeId = b2CreatePolygonShape(rb->bodyId, &def, &poly);
						}
					}
				}
				else continue;
			}

			b2Polygon poly = b2MakeOffsetBox(
				Units::ToMeters(box.halfExtents.x),
				Units::ToMeters(box.halfExtents.y),
				Units::ToMeters(box.offset),
				b2Rot_identity);

			b2Shape_SetPolygon(box.shapeId, &poly);
		}

		for (auto [entity, circle] : reg.view<CircleCollider>().each())
		{
			if (!circle.dirty)
				continue;
			circle.dirty = false;
			if (!b2Shape_IsValid(circle.shapeId)) {
				if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (auto* rb = reg.try_get<RigidBody>(entity)) {
						if (b2Body_IsValid(rb->bodyId)) {
							b2ShapeDef def = b2DefaultShapeDef();
							def.isSensor = circle.isSensor;
							b2Circle c = { Units::ToMeters(circle.offset), Units::ToMeters(circle.radius) };
							circle.shapeId = b2CreateCircleShape(rb->bodyId, &def, &c);
						}
					}
				}
				else continue;
			}

			b2Circle c = { Units::ToMeters(circle.offset), Units::ToMeters(circle.radius) };
			b2Shape_SetCircle(circle.shapeId, &c);
		}

		for (auto [entity, poly] : reg.view<PolygonCollider>().each())
		{
			if (!poly.dirty)
				continue;
			poly.dirty = false;
			if (!b2Shape_IsValid(poly.shapeId)) {
				if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (auto* rb = reg.try_get<RigidBody>(entity)) {
						if (b2Body_IsValid(rb->bodyId)) {
							b2Hull hull = b2ComputeHull(poly.vertices.data(), (int)poly.vertices.size());
							if (hull.count > 0)
							{
								b2ShapeDef def = b2DefaultShapeDef();
								def.isSensor = poly.isSensor;
								b2Polygon p = b2MakePolygon(&hull, 0.0f);
								poly.shapeId = b2CreatePolygonShape(rb->bodyId, &def, &p);
							}
						}
					}
				}
				else continue;
			}
			
			b2Hull hull = b2ComputeHull(poly.vertices.data(), (int)poly.vertices.size());
			if (hull.count > 0)
			{
				b2Polygon p = b2MakePolygon(&hull, 0.0f);
				b2Shape_SetPolygon(poly.shapeId, &p);
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

	void World::CreateShapes(Entity e, b2BodyId body)
	{
		if (!b2Body_IsValid(body))
			return;

		auto& reg = Engine.GetECS()->GetRegistry();
		if (auto* box = reg.try_get<BoxCollider>(e))
		{
			b2ShapeDef def = b2DefaultShapeDef();
			def.isSensor = box->isSensor;
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

	b2Vec2 World::GetVelocity(RigidBody rb) const
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	b2Vec2 World::AddVelocity(RigidBody rb, const b2Vec2 velocity)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, b2Body_GetLinearVelocity(rb.bodyId) + velocity);
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	b2Vec2 World::AddVelocityX(RigidBody rb, float velocityX)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, b2Vec2(b2Body_GetLinearVelocity(rb.bodyId).x + velocityX, b2Body_GetLinearVelocity(rb.bodyId).y));
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	b2Vec2 World::AddVelocityY(RigidBody rb, float velocityY)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, b2Vec2(b2Body_GetLinearVelocity(rb.bodyId).x, b2Body_GetLinearVelocity(rb.bodyId).y + velocityY));
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	b2Vec2 World::SetVelocity(RigidBody rb, const b2Vec2 velocity)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, velocity);
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	b2Vec2 World::SetVelocityX(RigidBody rb, float velocityX)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, b2Vec2(velocityX, b2Body_GetLinearVelocity(rb.bodyId).y));
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	b2Vec2 World::SetVelocityY(RigidBody rb, float velocityY)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return { 0.0f, 0.0f };
		b2Body_SetLinearVelocity(rb.bodyId, b2Vec2(b2Body_GetLinearVelocity(rb.bodyId).x, velocityY));
		return b2Body_GetLinearVelocity(rb.bodyId);
	}

	void World::ApplyForce(RigidBody rb, const b2Vec2 force, const b2Vec2 point, bool wake)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return;
		b2Body_ApplyForce(rb.bodyId, force, point, wake);
	}

	void World::ApplyImpulse(RigidBody rb, const b2Vec2 impulse, const b2Vec2 point, bool wake)
	{
		if (!b2Body_IsValid(rb.bodyId))
			return;
		b2Body_ApplyLinearImpulse(rb.bodyId, impulse, point, wake);
	}
};