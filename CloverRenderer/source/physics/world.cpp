#include "physics/world.hpp"
#include <imgui.h>
#include <core/engine.hpp>
#include <physics/physics_components.hpp>
#include "physics/units.hpp"
#include <core/transform.hpp>
#include <core/components.hpp>
#include <rendering/render_components.hpp>
#include <physics/sensor_component.hpp>

namespace clvr {
	World::World()
	{
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = m_gravity;
		m_worldId = b2CreateWorld(&worldDef);

		m_substeps = 4;

		title = "Physics World";
		priority = 90;
	}

	World::~World()
	{
		if (b2World_IsValid(m_worldId))
			b2DestroyWorld(m_worldId);
	}

	void World::Update(float dt)
	{
		// fixed time step
		SyncBodies();
		SyncColliders();

		constexpr float fixedStep = 1.0f / 60.0f;

		m_accumulator += std::fmin(dt, 0.25f);

		while (m_accumulator >= fixedStep)
		{
			if (Engine.GetEngineMode() == EngineMode::Playing)
			{
				UpdatePlayerMovement(fixedStep);            // same dt as the step it feeds
				b2World_Step(m_worldId, fixedStep, m_substeps);
				m_accumulator -= fixedStep;
				HandleContactEvents();
				HandleSensorEvents();
			}
			else if (Engine.GetEngineMode() == EngineMode::Editing)
			{
				// In edit mode, we still want to step the world to update the debug draw and any other physics-related things
				b2World_Step(m_worldId, 0, m_substeps);
				m_accumulator -= fixedStep;
			}
		}

		if (m_drawEnabled) {
			m_debugDraw.Begin();
			b2World_Draw(m_worldId, &m_debugDraw);
		}
		
		if (Engine.GetEngineMode() == EngineMode::Playing) {
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
	}

	void World::Render()
	{
		m_debugDraw.End();
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

		if (ImGui::Checkbox("Debug Draw", &m_drawEnabled)) {
			if (!m_drawEnabled) {
				m_debugDraw.End();
			}
		}

		ImGui::End();
	}

	void World::OnPlayStart() 
	{
		m_accumulator = 0.0f;
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = m_gravity;
		m_worldId = b2CreateWorld(&worldDef);
		// on play start, we need to create physics bodies for all entities that have a RigidBody component
		auto ecs = Engine.GetECS();
		for (auto [entity, rb, t] : ecs->GetRegistry().view<RigidBody, Transform>().each())
		{
			rb.bodyId = CreatePhysicsBody(rb, t);
			CreateShapes(entity, rb.bodyId);
		}
	}

	void World::OnPlayStop()
	{
		m_debugDraw.End();

		auto ecs = Engine.GetECS();
		for (auto [entity, rb, t] : ecs->GetRegistry().view<RigidBody, Transform>().each())
		{
			if (b2Body_IsValid(rb.bodyId))
			{
				b2DestroyBody(rb.bodyId);
				rb.bodyId = b2_nullBodyId;
			}
		}

		for (auto [entity, box] : ecs->GetRegistry().view<BoxCollider>().each())
		{
			if (b2Shape_IsValid(box.shapeId))
			{
				b2DestroyShape(box.shapeId, true);
				box.shapeId = b2_nullShapeId;
			}
		}

		for (auto [entity, circle] : ecs->GetRegistry().view<CircleCollider>().each())
		{
			if (b2Shape_IsValid(circle.shapeId))
			{
				b2DestroyShape(circle.shapeId, true);
				circle.shapeId = b2_nullShapeId;
			}
		}

		for (auto [entity, poly] : ecs->GetRegistry().view<PolygonCollider>().each())
		{
			if (b2Shape_IsValid(poly.shapeId))
			{
				b2DestroyShape(poly.shapeId, true);
				poly.shapeId = b2_nullShapeId;
			}
		}

		b2DestroyWorld(m_worldId);
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = m_gravity;
		m_worldId = b2CreateWorld(&worldDef);
	}

	json World::Save() {
		// Save gravity and substeps
		return json{
			{ "gravity", { m_gravity.x, m_gravity.y } },
			{ "substeps", m_substeps },
			{ "debugDraw", m_drawEnabled },
		};
	}

	void World::Load(const json& j) {
		// Load gravity and substeps
		b2DestroyWorld(m_worldId);
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = m_gravity;
		m_worldId = b2CreateWorld(&worldDef);

		if (j.contains("debugDraw") && j["debugDraw"].is_boolean()) {
			m_drawEnabled = j["debugDraw"].get<bool>();
		}
		if (j.contains("gravity") && j["gravity"].is_array() && j["gravity"].size() == 2) {
			m_gravity.x = j["gravity"][0].get<float>();
			m_gravity.y = j["gravity"][1].get<float>();
			if (b2World_IsValid(m_worldId)) {
				b2World_SetGravity(m_worldId, m_gravity);
			}
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
				//if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (b2Body_IsValid(rb.bodyId)) {
						b2ShapeDef def;
						CreateShape(box.material, box.isSensor, def, box.category);
						box.userData = std::make_unique<ShapeUserData>(ShapeUserData{entity});
						def.userData = box.userData.get();
						b2Polygon poly = b2MakeOffsetBox(
							Units::ToMeters(box.halfExtents.x),
							Units::ToMeters(box.halfExtents.y),
							Units::ToMeters(box.offset),
							b2Rot_identity);
						box.shapeId = b2CreatePolygonShape(rb.bodyId, &def, &poly);
						box.dirty = false;
					}
					else continue;
				//}
				//else continue;
			}

			if (b2Shape_IsSensor(box.shapeId) != box.isSensor) {
				//if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (b2Body_IsValid(rb.bodyId)) {
						b2ShapeDef def;
						CreateShape(box.material, box.isSensor, def, box.category);
						box.userData = std::make_unique<ShapeUserData>(ShapeUserData{ entity });
						def.userData = box.userData.get();
						b2Polygon poly = b2MakeOffsetBox(
							Units::ToMeters(box.halfExtents.x),
							Units::ToMeters(box.halfExtents.y),
							Units::ToMeters(box.offset),
							b2Rot_identity);
						b2DestroyShape(box.shapeId, true);
						box.shapeId = b2CreatePolygonShape(rb.bodyId, &def, &poly);
						box.dirty = false;
					}
					else continue;
				//}
				//else continue;
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
				//if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (b2Body_IsValid(rb.bodyId)) {
						b2ShapeDef def;
						CreateShape(circle.material, circle.isSensor, def, circle.category);
						circle.userData = std::make_unique<ShapeUserData>(ShapeUserData{ entity });
						def.userData = circle.userData.get();
						b2Circle c = { Units::ToMeters(circle.offset), Units::ToMeters(circle.radius) };
						circle.shapeId = b2CreateCircleShape(rb.bodyId, &def, &c);
						circle.dirty = false;
					}
					else continue;
				//}
				//else continue;
			}
			// check if sensor state has changed, if so, recreate the shape
			if (b2Shape_IsSensor(circle.shapeId) != circle.isSensor) {
				//if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (b2Body_IsValid(rb.bodyId)) {
						b2ShapeDef def;
						CreateShape(circle.material, circle.isSensor, def, circle.category);
						circle.userData = std::make_unique<ShapeUserData>(ShapeUserData{ entity });
						def.userData = circle.userData.get();
						b2Circle c = { Units::ToMeters(circle.offset), Units::ToMeters(circle.radius) };
						b2DestroyShape(circle.shapeId, true);
						circle.shapeId = b2CreateCircleShape(rb.bodyId, &def, &c);
						circle.dirty = false;
					}
					else continue;
				//}
				//else continue;
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
				//if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (b2Body_IsValid(rb.bodyId)) {
						b2Hull hull = b2ComputeHull(poly.vertices.data(), (int)poly.vertices.size());
						if (hull.count > 0)
						{
							b2ShapeDef def;
							CreateShape(poly.material, poly.isSensor, def, poly.category);
							poly.userData = std::make_unique<ShapeUserData>(ShapeUserData{ entity });
							def.userData = poly.userData.get();
							b2Polygon p = b2MakePolygon(&hull, 0.0f);
							poly.shapeId = b2CreatePolygonShape(rb.bodyId, &def, &p);
							poly.dirty = false;
						}
					}
					else continue;
				//}
				//else continue;
			}
			
			if (b2Shape_IsSensor(poly.shapeId) != poly.isSensor) {
				//if (Engine.GetEngineMode() == EngineMode::Playing) {
					if (b2Body_IsValid(rb.bodyId)) {
						b2Hull hull = b2ComputeHull(poly.vertices.data(), (int) poly.vertices.size());
						if (hull.count > 0)
						{
							b2ShapeDef def;
							CreateShape(poly.material, poly.isSensor, def, poly.category);
							poly.userData = std::make_unique<ShapeUserData>(ShapeUserData{ entity });
							def.userData = poly.userData.get();
							b2Polygon p = b2MakePolygon(&hull, 0.0f);
							b2DestroyShape(poly.shapeId, true);
							poly.shapeId = b2CreatePolygonShape(rb.bodyId, &def, &p);
							poly.dirty = false;
						}
					}
					else continue;
				//}
				//else continue;
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
				//if (Engine.GetEngineMode() == EngineMode::Playing) {
					rb.bodyId = CreatePhysicsBody(rb, t);
					CreateShapes(entity, rb.bodyId);
				//}
				//else continue;
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

	void World::CreateShape(PhysicsMaterial mat, bool isSensor, b2ShapeDef& out, CollisionFilters::CC category) {
		assert(CollisionFilters::IsSingleBit(category) && "Collision category must be a single bit.");
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
		
		out.filter.categoryBits = CollisionFilters::Bits(category);
		out.filter.maskBits = CollisionFilters::MaskFor(category);
	}

	void World::CreateShapes(Entity e, b2BodyId body)
	{
		if (!b2Body_IsValid(body))
			return;

		auto& reg = Engine.GetECS()->GetRegistry();
		if (auto* box = reg.try_get<BoxCollider>(e))
		{
			b2ShapeDef def;
			CreateShape(box->material, box->isSensor, def, box->category);
			box->userData = std::make_unique<ShapeUserData>(ShapeUserData{ e });
			def.userData = box->userData.get();
			b2Polygon poly = b2MakeOffsetBox(Units::ToMeters(box->halfExtents.x),
				Units::ToMeters(box->halfExtents.y),
				{ Units::ToMeters(box->offset.x), Units::ToMeters(box->offset.y) },
				b2Rot_identity);
			box->shapeId = b2CreatePolygonShape(body, &def, &poly);
		}

		if (auto* circle = reg.try_get<CircleCollider>(e))
		{
			b2ShapeDef def;
			CreateShape(circle->material, circle->isSensor, def, circle->category);
			circle->userData = std::make_unique<ShapeUserData>(ShapeUserData{ e });
			def.userData = circle->userData.get();
			b2Circle c = { Units::ToMeters(circle->offset), Units::ToMeters(circle->radius) };
			circle->shapeId = b2CreateCircleShape(body, &def, &c);
		}

		if (auto* poly = reg.try_get<PolygonCollider>(e))
		{
			b2Hull hull = b2ComputeHull(poly->vertices.data(), (int)poly->vertices.size());
			if (hull.count > 0)
			{
				b2ShapeDef def;
				CreateShape(poly->material, poly->isSensor, def, poly->category);
				poly->userData = std::make_unique<ShapeUserData>(ShapeUserData{ e });
				def.userData = poly->userData.get();
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

	void World::DrawShape(b2ShapeId shape, b2HexColor color)
	{
		m_debugDraw.DrawShape(shape, color);
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

	void World::HandleContactEvents()
	{
		b2ContactEvents contactEvents = b2World_GetContactEvents(m_worldId);
		for (int i = 0; i < contactEvents.beginCount; ++i)
		{
			const b2ContactBeginTouchEvent& event = contactEvents.beginEvents[i];

			auto shapeA = event.shapeIdA;
			auto shapeB = event.shapeIdB;
			if (!b2Shape_IsValid(shapeA) || !b2Shape_IsValid(shapeB))
				continue;
			auto bodyA = b2Shape_GetBody(shapeA);
			auto bodyB = b2Shape_GetBody(shapeB);
			if (!b2Body_IsValid(bodyA) || !b2Body_IsValid(bodyB))
				continue;

			// Handle contact begin event
			// You can add your custom logic here
		}
		for (int i = 0; i < contactEvents.hitCount; ++i)
		{
			const b2ContactHitEvent& event = contactEvents.hitEvents[i];

			auto shapeA = event.shapeIdA;
			auto shapeB = event.shapeIdB;
			if (!b2Shape_IsValid(shapeA) || !b2Shape_IsValid(shapeB))
				continue;
			auto bodyA = b2Shape_GetBody(shapeA);
			auto bodyB = b2Shape_GetBody(shapeB);
			if (!b2Body_IsValid(bodyA) || !b2Body_IsValid(bodyB))
				continue;

			// Handle contact hit event
			// You can add your custom logic here
		}
		for (int i = 0; i < contactEvents.endCount; ++i)
		{
			const b2ContactEndTouchEvent& event = contactEvents.endEvents[i];

			auto shapeA = event.shapeIdA;
			auto shapeB = event.shapeIdB;
			if (!b2Shape_IsValid(shapeA) || !b2Shape_IsValid(shapeB))
				continue;
			auto bodyA = b2Shape_GetBody(shapeA);
			auto bodyB = b2Shape_GetBody(shapeB);
			if (!b2Body_IsValid(bodyA) || !b2Body_IsValid(bodyB))
				continue;

			// Handle contact end event
			// You can add your custom logic here
		}
	}

	void World::HandleSensorEvents()
	{
		// first check for stay events in the current frame, if so, call OnStay for the sensor component
		auto& registry = Engine.GetECS()->GetRegistry();

		for (auto [entity, sensor, box] : registry.view<SensorComponent, BoxCollider>().each())
		{
			if (!b2Shape_IsValid(box.shapeId))
				continue;
			sensor.OnStay(registry, entt::null);
		}

		for (auto [entity, sensor, circle] : registry.view<SensorComponent, CircleCollider>().each())
		{
			if (!b2Shape_IsValid(circle.shapeId))
				continue;
			sensor.OnStay(registry, entt::null);
		}

		for (auto [entity, sensor, poly] : registry.view<SensorComponent, PolygonCollider>().each())
		{
			if (!b2Shape_IsValid(poly.shapeId))
				continue;
			sensor.OnStay(registry, entt::null);
		}

		// then check for begin and end events in the current frame, if so, call OnBegin/OnEnd for the sensor component
		b2SensorEvents events = b2World_GetSensorEvents(m_worldId);
		for (int i = 0; i < events.beginCount; ++i)
		{
			const b2SensorBeginTouchEvent& event = events.beginEvents[i];

			auto sensorId = event.sensorShapeId;
			auto visitorId = event.visitorShapeId;
			if (!b2Shape_IsValid(sensorId) || !b2Shape_IsValid(visitorId))
				continue;
			auto sensorBody = b2Shape_GetBody(sensorId);
			auto visitorBody = b2Shape_GetBody(visitorId);
			if (!b2Body_IsValid(sensorBody) || !b2Body_IsValid(visitorBody))
				continue;

			// Handle sensor begin touch event
			// You can add your custom logic here

			auto* sData = static_cast<ShapeUserData*>(b2Shape_GetUserData(sensorId));
			auto* vData = static_cast<ShapeUserData*>(b2Shape_GetUserData(visitorId));
			if (!sData || !vData)
				continue;

			Entity sEntity = sData->entity;
			Entity vEntity = vData->entity;

			if (!registry.valid(sEntity) || !registry.valid(vEntity))
				continue;

			// check if the sensor has a SensorComponent, if so, call its OnBegin method
			auto* sensorComp = registry.try_get<SensorComponent>(sEntity);
			if (sensorComp)
			{
				sensorComp->OnBegin(registry, vEntity);
			}
		}
		for (int i = 0; i < events.endCount; ++i)
		{
			const b2SensorEndTouchEvent& event = events.endEvents[i];

			auto sensorId = event.sensorShapeId;
			auto visitorId = event.visitorShapeId;
			if (!b2Shape_IsValid(sensorId) || !b2Shape_IsValid(visitorId))
				continue;
			auto sensorBody = b2Shape_GetBody(sensorId);
			auto visitorBody = b2Shape_GetBody(visitorId);
			if (!b2Body_IsValid(sensorBody) || !b2Body_IsValid(visitorBody))
				continue;

			// Handle sensor end touch event
			// You can add your custom logic here

			auto* sData = static_cast<ShapeUserData*>(b2Shape_GetUserData(sensorId));
			auto* vData = static_cast<ShapeUserData*>(b2Shape_GetUserData(visitorId));
			if (!sData || !vData)
				continue;

			Entity sEntity = sData->entity;
			Entity vEntity = vData->entity;

			if (!registry.valid(sEntity) || !registry.valid(vEntity))
				continue;

			// check if the sensor has a SensorComponent, if so, call its OnEnd method
			auto* sensorComp = registry.try_get<SensorComponent>(sEntity);
			if (sensorComp)
			{
				sensorComp->OnEnd(registry, vEntity);
			}
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