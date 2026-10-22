#include "physics/world_helpers.hpp"
#include "physics/world.hpp"
#include "physics/physics_components.hpp"
#include "physics/sensor_component.hpp"
#include <core/engine.hpp>
#include <core/ecs.hpp>

namespace clvr
{
	SensorComponent& CreateBoxSensor(World& world, Entity entity, float x, float y)
	{
		auto ecs = Engine.GetECS();
		auto& reg = ecs->GetRegistry();

		auto& rb = ecs->CreateComponent<RigidBody>(entity);
		rb.bodyType = b2_staticBody; // Sensors are usually static
		
		auto& box = ecs->CreateComponent<BoxCollider>(entity);
		box.halfExtents = b2Vec2(x, y); // Default half extents
		box.isSensor = true; // Mark as sensor

		PhysicsMaterial material = PhysicsMaterial{};
		material.density = 0.0f; // Sensors typically have no density
		material.friction = 0.0f; // No friction for sensors
		material.restitution = 0.0f; // No restitution for sensors
		material.enableSensorEvents = true; // Enable sensor events
		material.enableContactEvents = false; // No contact events for sensors
		material.enableHitEvents = false; // No hit events for sensors
		
		box.material = material;

		b2ShapeDef boxShapeDef;
		world.CreateShape(box.material, true, boxShapeDef);

		return ecs->CreateComponent<SensorComponent>(entity);
	}

	SensorComponent& CreateCircleSensor(World& world, Entity entity, float radius)
	{
		auto ecs = Engine.GetECS();
		auto& reg = ecs->GetRegistry();

		auto& rb = ecs->CreateComponent<RigidBody>(entity);
		rb.bodyType = b2_staticBody; // Sensors are usually static

		auto& circle = ecs->CreateComponent<CircleCollider>(entity);
		circle.radius = radius; // Default half extents
		circle.isSensor = true; // Mark as sensor

		PhysicsMaterial material = PhysicsMaterial{};
		material.density = 0.0f; // Sensors typically have no density
		material.friction = 0.0f; // No friction for sensors
		material.restitution = 0.0f; // No restitution for sensors
		material.enableSensorEvents = true; // Enable sensor events
		material.enableContactEvents = false; // No contact events for sensors
		material.enableHitEvents = false; // No hit events for sensors

		circle.material = material;

		b2ShapeDef circleShapeDef;
		world.CreateShape(circle.material, true, circleShapeDef);

		return ecs->CreateComponent<SensorComponent>(entity);
	}
}