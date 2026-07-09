#include "core/ecs.hpp"

using namespace clvr;
using namespace std;

constexpr float kMaxDeltaTime = 1.0f / 30.0f;

EntityComponentSystem::EntityComponentSystem() = default;

EntityComponentSystem::~EntityComponentSystem() = default;

void EntityComponentSystem::DeleteEntity(Entity e)
{
    assert(m_registry.valid(e));

    // mark this entity for deletion
    m_registry.emplace_or_replace<Delete>(e);
}

void EntityComponentSystem::UpdateSystems(float dt)
{
    dt = std::min(dt, kMaxDeltaTime);
    for (auto& s : m_systems) s->Update(dt);
}

void EntityComponentSystem::RenderSystems()
{
    for (auto& s : m_systems) s->Render();
}

void EntityComponentSystem::InspectSystems(float dt)
{
	for (auto& s : m_systems) s->Inspect(dt);
}

void EntityComponentSystem::RemovedDeleted()
{
    auto& deleteStorage = m_registry.storage<Delete>();
    while (!deleteStorage.empty())
    {
        // Destroying entities may enqueue more Deletes, hence the loop
        const auto del = m_registry.view<Delete>();
        m_registry.destroy(del.begin(), del.end());
    }
}
