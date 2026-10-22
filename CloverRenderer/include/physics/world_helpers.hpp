#pragma once

#include "physics/world.hpp"
#include "physics/physics_components.hpp"
#include "physics/sensor_component.hpp"

namespace clvr
{
	// Helper function to create a Box2D shape with a sensor
	SensorComponent& CreateBoxSensor(World& world, Entity entity, float x, float y);
	SensorComponent& CreateCircleSensor(World& world, Entity entity, float radius);
	//SensorComponent& CreatePolygonSensor(World& world, Entity entity);
}