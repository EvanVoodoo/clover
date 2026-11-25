#pragma once

#include "physics/world.hpp"
#include "physics/physics_components.hpp"
#include "physics/sensor_component.hpp"

namespace clvr
{
	// Helper function to create a Box2D shape with a sensor
	BoxCollider& CreateBoxCollider(World& world, Entity entity, float x, float y, CollisionFilters::CC category = CollisionFilters::CC::Default);
	CircleCollider& CreateCircleCollider(World& world, Entity entity, float radius, CollisionFilters::CC category = CollisionFilters::CC::Default);
	SensorComponent& CreateBoxSensor(World& world, Entity entity, float x, float y, CollisionFilters::CC category = CollisionFilters::CC::Default);
	SensorComponent& CreateCircleSensor(World& world, Entity entity, float radius, CollisionFilters::CC category = CollisionFilters::CC::Default);
	//SensorComponent& CreatePolygonSensor(World& world, Entity entity);
}